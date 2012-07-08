/*
 *   Copyright (C) 2012 by Mario St-Gelais
 * 
 *   This file is part of NFFM.
 *
 *   NFFM is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   NFFM is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with NFFM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "nffm.h"

directories DoDirectoryList(char adir[], char *directory_list[], char *file_list[], options opt)
{
    /*adir is the full path to the directory
     * Directory_list are the directories within adir
     * file_list are the files within adir
    */
    struct dirent **namelist;
    struct stat statbuf;
    directories dirs;
    char str[STRLEN];
    int n;
    int m;
    int file_index=0;
    int dir_index=0;
    dirs.dir_size=0;

    addslash(adir);
    n = scandir(adir, &namelist, 0, alphasort);
    //n = scandir(adir, &namelist, filterfile, alphasort);
    if (n < 0)
        printf("scandir %s %s\n",adir, sys_errlist[errno]);
    else 
    {
        for (m=0;m<n;m++)
        {
            snprintf(str, STRLEN, "%s%s",adir,namelist[m]->d_name);
            if(lstat(str, &statbuf)!=0)
            {
                perror("In DoDirectories File not found");
                exit(1);
            }
            if(S_ISDIR(statbuf.st_mode)==1)
            {
                if(!opt.show_hidden && isHiddenFile(namelist[m]->d_name))
                    continue;
                directory_list[dir_index++]= strdup(namelist[m]->d_name);
                if(dir_index>MAXDIRLIST)
                {
                    echo();
                    endwin();
                    perror("Directory too long");
                    exit(EXIT_FAILURE);
                }

            }
            else
            {
                if(!opt.show_hidden && isHiddenFile(namelist[m]->d_name))
                    continue;
                if(opt.file_ext[0]!='\0' && !ends_with(namelist[m]->d_name,opt.file_ext) )
                    continue;
                if(opt.file_begin[0]!='\0' && !begins_with(namelist[m]->d_name,opt.file_begin) )
                    continue;
                file_list[file_index++]= strdup(namelist[m]->d_name);
                dirs.dir_size+=statbuf.st_size;
            }
            free(namelist[m]);
        }
       free(namelist);
       namelist=NULL;
       directory_list[dir_index]='\0';
       file_list[file_index]='\0';
    }
    dirs.dir_count=dir_index;
    dirs.file_count=file_index;
    strcpy(dirs.dirname, adir);
    logger("\nDircount ");
    logger(delimLong(dirs.dir_count));
    return dirs;
}

void logger(const char *logger)
{
    FILE *flog;
    flog=file_open("/tmp/log.txt","a");
    fprintf(flog, "%s", logger);
    fclose(flog);
}

struct stat fileStat(char filepath[])
{
    struct stat statbuf;
    if(lstat(filepath, &statbuf)!=0)
    {
        printf("<<%s>>\n",filepath);
        perror("In fileStat, File not found");
        exit(1);
    }
    return statbuf;
}

int addslash(char adir[])
{
    int length;
    length=strlen(adir);
    int i, j;
    if(adir[0]=='/' && adir[length-1]=='/')
        return 0;
    j=0;
    if(adir[length-1]!='/')
    {
        strncat(adir,"/",1);
        length++;
    }
    char slashed[length];
    if(adir[0]!='/')
    {
        slashed[0]='/';
        j++;
    }
    for(i=j; i<=length; i++)
        slashed[i]=adir[i-j];
    slashed[i]='\0';
    strcpy(adir, slashed);
    return 0;
}

int split(char delim, char *string)
{
    if(strcmp(string,"/")==0)
        return 0;
    int lastPos;
    lastPos=strlen(string);
    int pos;
    int lastDelim;
    int i;
    char c;
    i=0;
    for(pos=0;pos<lastPos-1;pos++)
    {
        c=string[pos];
        if(c==delim)
        {
            i++;
            lastDelim=pos;
        }
    }
    string[lastDelim]='\0';
    return lastDelim;
}

char *delimStr(char nbr[])
{
    int nbr_length=strlen(nbr);
    int nbrfmt_length;
    nbrfmt_length=nbr_length+nbr_length/3;
    if(nbr_length%3==0)
        nbrfmt_length-=1;
    char *fmtnbr=malloc(nbrfmt_length+1);
    int rpos=0;
    while(nbrfmt_length>=0)
    {
        fmtnbr[nbrfmt_length--]=nbr[nbr_length--];
        if (rpos%3==0 && rpos>0 && nbrfmt_length>0)
            fmtnbr[nbrfmt_length--]=',';
        rpos++;
    }
    return fmtnbr;
}

char *delimLong(long nbr)
{
    int i=0;
    long divider;
    divider=nbr;
    
    while((divider/=10)>=1)
        i++;
    char *nbrstr=malloc(i+2);
    char *longstr;
    sprintf(nbrstr,"%ld",nbr);
    longstr=delimStr(nbrstr);
    free(nbrstr);
    nbrstr=NULL;
    return longstr;
}

char *join(char dir[], char file[])
{
    int dir_length;
    int file_length;
    
    dir_length=strlen(dir);
    file_length=strlen(file);
    if(dir_length<=0 || file_length<=0)
    {
        perror("In join");
        exit(1);
    }
    char *joined=malloc((dir_length+file_length+1));
    if(joined==NULL)
    {
        perror("In malloc join function");
        exit(1);
    }
    strcpy(joined,dir);
    addslash(joined);
    strcat(joined,file);
    return joined;
}

bool isHiddenFile(char s[])
{
    if(s[0]=='.')
        return true;
    return false;
}

FILE * file_open(const char *filename, const char *mode)
{
    FILE * fp;
    if ((fp = fopen(filename, mode))==NULL)
    {
        printf("Cannot open file %s for mode%s",filename, mode);
        echo();
        exit(1);
    }
    return fp;
}

char * getFileExtension(const char *filename)
{
    int slen;  //filename Length
    int elen=0;  //file extension lenght
    char *p;
    char *extension = malloc(5);
    slen=strlen(filename);
    p=strdup(filename);
    while(slen>0 && p[slen-1]!='.')
    {
        elen++;
        extension=&p[slen-1];
        slen--;
    }
    if(slen==0)
        strcpy(extension,"default");
    return extension;
}

appCommand getCommand(const char *extension)
{
    FILE *f;
    const int linelength=80;
    char line[80];
    char config_path[80];
    appCommand ac;
    ac.extension[0]='\0';
    strcpy(config_path, GetUserDir());
    addslash(config_path);
    strcat(config_path, CONF_FILE);

    f=file_open(config_path,"r");
    char scan_extension[10];
    
    while(fgets(line, linelength,f))
    {
        sscanf(line,"%s",scan_extension);
        if(strcmp(scan_extension,extension)==0) //extension found
        {
            if(sscanf(line,"%s%s%s%s", ac.extension, ac.path, ac.arg, ac.execarg)==4)
                return ac;
            else if(sscanf(line, "%s%s", ac.extension, ac.path)==2)
            {
                strcpy(ac.arg,"\0");
                strcpy(ac.execarg,"\0");
                return ac;
            }
        }
    }
    fclose(f);
    return ac;
}

bool ends_with(const char *string, const char *ends)
{
    char *p;
    p=rindex(string, '.');
    if(p==NULL)
        return false;

    return (strcmp(++p,ends)==0) ? true : false;
}

bool begins_with(const char *string, const char *begins)
{
    int i=0;
    int matchLength=strlen(begins);
    while((begins[i]==string[i]))
        i++;
    if(i==matchLength)
        return true;
    return false;
}

int filterfile(const struct dirent *d)
{
    return ends_with(d->d_name,".jpg");
}
int ReadLine(char c, char aLine[])
{
    int lineLength=strlen(aLine);
    switch(c)
    {
        case '\177':  //Backspace
            aLine[lineLength-1]='\0';
            break;
        case '\033':  //Escape
            aLine[0]='\0';
            return -1;
        default:
            aLine[lineLength++]=c;
            aLine[lineLength]='\0';
            break;
    }
    return strlen(aLine);
}
options setFileFilter(options opt)
{
    char c;
    char ext[10]={};
    wbkgd(winfooter, COLOR_PAIR(4));
    mvwprintw(winfooter, 0, 0, "%s", "Enter file extension: ");
    wrefresh(winfooter);
    while((c=wgetch(winfooter))!='\n')
    {
        ReadLine(c, ext);
        werase(winfooter);
        mvwprintw(winfooter, 0, 0, "%s %s", "Enter file extension: ", ext);
        wrefresh(winfooter);
    }
    werase(winfooter);
    wbkgd(winfooter, COLOR_PAIR(1));
    wrefresh(winfooter);
    strcpy(opt.file_ext, ext);
    return opt;
}

options setFileBeginOption(options opt)
{
    char c;
    char begin[MAXBEGIN+1]={0};
    wbkgd(winfooter, COLOR_PAIR(4));
    mvwprintw(winfooter, 0, 0, "%s", "Enter string file begins with: ");
    wrefresh(winfooter);
    while((c=wgetch(winfooter))!='\n')
    {
        ReadLine(c, begin);
        werase(winfooter);
        mvwprintw(winfooter, 0, 0, "%s %s", "Enter string file begins with: ", begin);
        wrefresh(winfooter);
    }
    werase(winfooter);
    wbkgd(winfooter, COLOR_PAIR(1));
    wrefresh(winfooter);
    strcpy(opt.file_begin, begin);
    return opt;
    
}
char *GetUserDir(void)
{
    struct passwd *pwd;
    uid_t uid;
    uid=geteuid();
    pwd=getpwuid(uid);
    return (pwd==NULL) ? NULL : pwd->pw_dir;
}

void message(char *msg)
{
    wbkgd(winfooter, COLOR_PAIR(4));
    mvwprintw(winfooter, 0, 0, "%s", msg);
    wrefresh(winfooter);
    wgetch(winmenu);
    werase(winfooter);
    wbkgd(winfooter, COLOR_PAIR(1));
    wrefresh(winfooter);
}
int xdgFile(char *file)
{
    pid_t p;
    int status;
    appCommand ac;
    ac=getCommand(getFileExtension(file));
    if(strcmp(ac.extension,"\0")==0)
        return -1;
    switch(p=fork())
    {
        case -1: //error
            return -1;
        case 0:
            close(1);
            close(2);
            if(strcmp(ac.arg,"\0")!=0)
                execl(ac.path, ac.path, ac.arg, ac.execarg, file, (char *) NULL);
            else
                execl(ac.path, ac.path, file, (char *) NULL);
        default: //parent
            logger("\noups");
            if(waitpid(p, &status, 0)==1)
                return -1;
            else
                return status;
    }
}

void markOneMoreFile(struct filemarker **filelist, char *filepath)
{
    if (findMarkedFile(*filelist, filepath))
    {
        UnmarkFile(filelist, filepath);
        return;
    }
    struct filemarker *new_marker;
    new_marker=malloc(sizeof(struct filemarker));
    if (new_marker==NULL)
    {
        perror("Mark selected file failed");
        exit(EXIT_FAILURE);
    }
    strcpy(new_marker->fullpath, filepath);
    new_marker->next=*filelist;
    //logger((char *)new_marker->next);
    *filelist=new_marker;
}

int deleteMarkedFile(struct filemarker **f)
{
    int i=0;
    struct filemarker *p;
    for(p=*f; p!=NULL; p=p->next)
    {
        i++;
        deleteFile(p->fullpath, CONFIRMDELETEMANY);
        //UnmarkFile(f, p->fullpath);
        //free(p->fullpath);
    }
    free(p);
    return i;
}

bool UnmarkFile(struct filemarker **filelist, const char *filepath)
{
    logger("\n\nEntering...");
    logger(__func__);
    logger(filepath); 
    struct filemarker *current, *previous;
    current=*filelist;
    previous=NULL;
    for (;;)
    {
        if(strcmp(current->fullpath, filepath)!=0)
        {
            previous=current;
            current = current->next;
        }
        else
            break;
    }
    if (current == NULL)
        return false;
    if (previous == NULL)
        *filelist = current->next;
    else
        previous->next=current->next;
    free(current);
    logger("\nExiting...");
    logger(__func__);
    return true;
}

bool findMarkedFile(struct filemarker *f, char *filepath)
{
    struct filemarker *p;
    for(p=f; p!=NULL; p=p->next)
    {
        if(strcmp(p->fullpath,filepath)==0)
        {
            logger("File Match:");
            logger(p->fullpath);
            return true;
        }
    }
    return false;
}

int getNumber(WINDOW *w)
/*Return an up to 3 digit long number read from stdin*/
{
    char nbr[MAXDIGIT+1];
    char c;
    int i;
    i=0;
    while((c=wgetch(w))!='\n' && i<MAXDIGIT)
    {
        if(c>='0' && c<='9')
            nbr[i++]=c;
    }
    nbr[i]='\0';
    return strtol(nbr, NULL, 10);
}

void reverseColor(WINDOW *w, cursor c, char *item)
{
    wattron(w, COLOR_PAIR(2));
    wattron(w, A_REVERSE);
    mvwprintw(w, c.arrowcounter, 0, "%-3d>%-27s", c.menuitem, item);
    wrefresh(w);
    wattroff(w, A_REVERSE);

}

void normalColor(WINDOW *w, cursor c, char *item)
{
    mvwprintw(w, c.arrowcounter, 0, "%-3d>%-27s", c.menuitem, item);
    wrefresh(w);
}

int displayList(struct filemarker *f)
{
    struct filemarker *fm;
    int i=0;
    werase(wintransit);
    for(fm=f;fm!=NULL; fm=fm->next)
    {
       mvwprintw(wintransit,i++,0,"[%-33s]",fm->fullpath); 
    }
    wrefresh(wintransit);
    return i;
}

int drawmenu(char *list[], char *item, WINDOW *w, int fromline)
{
    int printline=0;
    int dir_index=fromline;
    int h;
    werase(w);
    if (w==winscrollable)
        h=FILEMAX;
    else
        h=MENUMAX;
    while (list[dir_index]!='\0' && printline<h)
	{
        if(strcmp(list[dir_index], item)==0)
            wattron(w, A_REVERSE);
        wattron(w, COLOR_PAIR(2));
        mvwprintw(w, printline++, 0, "%-3d%-37s", dir_index, list[dir_index]);
        wattroff(w, A_REVERSE);
        wattroff(w, COLOR_PAIR(2));
        dir_index++;
	}
    wrefresh(w);
    return 0;
}

void refreshDirInfo(directories dirs)
{
    char *fmt_size;
    fmt_size=delimLong(dirs.dir_size);
    werase(windirinfo);
    mvwprintw(windirinfo, 0, 0, "%3d Directories",dirs.dir_count);
    mvwprintw(windirinfo, 1, 0, "%3d Files",dirs.file_count);
    if(FORMATTHOUSAND)
        mvwprintw(windirinfo, 1, 10, " %s Bytes",fmt_size);
    else
        mvwprintw(windirinfo, 1, 10, "%3d Bytes",dirs.dir_size);
    free(fmt_size);
    fmt_size=NULL;
    wrefresh(windirinfo);
}

char *dtg(time_t *tm)
{
    char *t=malloc(17); //2010-06-30 08:23
    strftime(t, 17, "%F %H:%S", localtime(tm));
    return t;
}

void refreshFileInfo(char currentDir[], char currentFile[])
{
    struct stat fileinfo;
    char *t;
    char *fmt_size;
    char *filepath;
    filepath = join(currentDir, currentFile);
    fileinfo = fileStat(filepath);
    fmt_size=delimLong((long)fileinfo.st_size);
    mvwprintw(winfileinfo,0,0,"%24s",currentFile);
    mvwprintw(winfileinfo,1,0,"%18s Bytes",fmt_size);
    t=dtg(&fileinfo.st_atime);
    mvwprintw(winfileinfo,2,0,"Access: %16s",t);
    free(t);
    t=NULL;
    t=dtg(&fileinfo.st_mtime);
    mvwprintw(winfileinfo,3,0,"Modif:  %16s",t);
    free(t);
    t=NULL;
    t=dtg(&fileinfo.st_ctime);
    mvwprintw(winfileinfo,4,0,"Created:%16s",t);
    free(t);
    t=NULL;
    free(fmt_size);
    fmt_size=NULL;
    free(filepath);
    filepath=NULL;
    wrefresh(winfileinfo);
}

cursor setCursor(int direction, int selection, cursor c) 
{
    logger("\nCursor Before");
    logger(printCursor(c));
    switch (direction)
    {
        case DOWN:
            if(selection>=c.linecount)//Bottom of dir or file list 
            {
                selection=0;
                c.linemarker=0;
            }
            else if(selection>=c.linemax)
            {
                if(selection>=c.linemax+c.linemarker)
                    c.linemarker+=c.linemax;
            }
            /*
            else
            {
                perror("Unhandled setCurosr");
                echo();
                endwin();
                exit(1);
            }
            */
            break;
        case UP:
            if(selection<0) //Beginning of dir or file list
            {
                selection=c.linecount -1;
                c.linemarker=c.linecount - c.linemax;
                if(c.linemarker<0)
                    c.linemarker=0;
                //logger("selection<0");
            }
            else if(selection<c.linemarker)
            {
                c.linemarker-=c.linemax;
                if(c.linemarker<0)
                    c.linemarker=0;
                //logger("selection<c.linemarker");
            }
            break;
        case SAME:
            break;
    }
    c.menuitem=c.arrowcounter=selection;
    logger("\nCursor After");
    logger(printCursor(c));
    return c;
}

char *printCursor(cursor c)
    /*This is just for debugging purpises*/
{
    char *pc=malloc(250);
    sprintf(pc, "menuitem = %d\n diritemold = %d\n fileitemold = %d\n winmarker = %d\n linemarker = %d\n linemarkerfile = %d\n linemarkerdir = %d\n linemax = %d\n linecount = %d\n arrowcounter = %d\n arrowcounterdir = %d\n arrowcounterfile = %d\n",
            c.menuitem, 
            c.diritemold, 
            c.fileitemold, 
            c.winmarker, 
            c.linemarker, 
            c.linemarkerfile,
            c.linemarkerdir,
            c.linemax,
            c.linecount,      
            c.arrowcounter,    
            c.arrowcounterdir,
            c.arrowcounterfile);
    return pc; 
}
int renameSelectedFile(const char *currentPath, const char *oldName)
{
    char c;
    char newName[80]={0};
    char *oldPath;
    char *newPath;
    int i=0;
    wbkgd(winfooter, COLOR_PAIR(4));
    mvwprintw(winfooter, 0, 0, "%s", "Enter new file name: ");
    wrefresh(winfooter);
    while((c=wgetch(winfooter))!='\n')
    {
        if(ReadLine(c, newName))
        {
            werase(winfooter);
            mvwprintw(winfooter, 0, 0, "%s %s", "Enter new file name: ", newName);
            wrefresh(winfooter);
        }
    }
    werase(winfooter);
    wbkgd(winfooter, COLOR_PAIR(1));
    wrefresh(winfooter);
    oldPath=malloc(strlen(currentPath)+strlen(oldName)+1);
    newPath=malloc(strlen(currentPath)+strlen(newName)+1);
    strcpy(oldPath,currentPath);
    strcat(oldPath,oldName);
    strcpy(newPath,currentPath);
    strcat(newPath,newName);

    rename(oldPath,newPath);
    free(oldPath);
    free(newPath);
}

int deleteFile(const char *filepath, bool confirmDeleteMany)
{
    //logger("\n\nEntering..");
    //logger(__func__);
    char c;
    int deleteOk=-1;
    if(confirmDeleteMany)
    {
        wbkgd(winfooter, COLOR_PAIR(4));
        mvwprintw(winfooter, 0, 0, "%s %s\?", "Do you really want to DELETE FILE ", filepath);
        wrefresh(winfooter);
        while((c=wgetch(winmenu))!='y' && c!='Y' && c!='n' && c!='N')
            ;
        werase(winfooter);
        switch (c)
        {
            case 'y':
            case 'Y':
                deleteOk=remove(filepath);
                mvwprintw(winfooter, 0, 0, "%s %c", "DELETE FILE ", c);
                break;
            default:
                mvwprintw(winfooter, 0, 0, "%s", "Action cancelled");
                break;
        }
        wrefresh(winfooter);
        sleep(0.5);
        werase(winfooter);
        wbkgd(winfooter, COLOR_PAIR(1));
        wrefresh(winfooter);
    }
    else
    {
        deleteOk=remove(filepath);
    }
    return deleteOk;
}

int main(void)
{
	int key=0;
    cursor cursor;
	cursor.menuitem = 0;       //index of directory or filelist
	cursor.diritemold = 0;    //index to retain menuitem when tabbing
	cursor.fileitemold = 0;    //index to retain menuitem when tabbing
    cursor.winmarker = AT_DIR; //-1=AT_DIR, 1=AT_FILE
    cursor.linemarker = 0;     //Goes from 0 to linecount
    cursor.linemarkerfile = 0;
    cursor.linemarkerdir = 0;  
    cursor.linemax = MENUMAX;  //or FILEMAX 
    cursor.linecount = 0;      //from directories or files list length
    cursor.arrowcounter= 0;    //Switch from arrowcounterdir or arrowcounterfile. 0 to linemax
    cursor.arrowcounterdir=0;  //The screen row of the highlighted directory
    cursor.arrowcounterfile=0; //The screen row of the highlighted file

    options opt;
    opt.show_hidden=0;
    opt.file_ext[0]='\0';
    opt.file_begin[0]='\0';

    int maxwidth, maxheight;
    char currentDir[200]={0};   //To keep track of the current full path dir
    char currentFile[200]={0};  //To keep track of the current file
    char *dirlist[MAXDIRLIST];    //The child directories of the current directory
    char *filelist[MAXFILELIST];   //The files of the current directory
    char **activelist;      //dirlist or dirfilt
    char msg[100];              //To display messages on status bar
    //struct winsize ws;
    directories dirs;

	initscr();
    start_color();
    init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_RED);  //For warning purposes
    getmaxyx(stdscr, maxheight, maxwidth);
 
    winmenu=newwin(MENUHT, MENUW, 1, 0);
    windirinfo=newwin(5, MENUW, MENUHT+2, 0);
    winfileinfo=newwin(5, MENUW, MENUHT+2+3, 0);
    winheader=newwin(1,maxwidth,0,0);
    winfooter=newwin(1,maxwidth,maxheight-1,0);
    winscrollable=newwin(MENUHT, WINFILEW, 1,MENUW+1);
    wintransit=newwin(maxheight,WINTRANSITW,1,MENUW+1+WINFILEW+1);

    dirs=DoDirectoryList(ROOT, dirlist, filelist, opt); 
    cursor.linecount=dirs.dir_count;
    activelist=dirlist;
    refreshDirInfo(dirs);
    strcpy(currentDir,ROOT);
    wattron(winheader,COLOR_PAIR(3));
    wattron(winmenu, A_BOLD);
    currentwin=winmenu;
    drawmenu(activelist, activelist[0], winmenu, cursor.linemarker);
	keypad(currentwin, true);
	noecho();

	do
	{
        /*
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) ==-1)
        {
            perror("ioctl");
        }
        else
        {
            resizeterm(ws.ws_row, ws.ws_col);
            winscrollable=newwin(maxheight, ws.ws_col-MENUW,0,MENUW);
            wresize(winscrollable,maxheight, ws.ws_col-MENUW);
            drawmenu(filelist, filelist[cursor.menuitem], winscrollable, cursor.linemarkerfile);
        } 
        */
        key=wgetch(winmenu);
        switch(key)
        {
            case SELECTBEGINWITH:
                if (cursor.winmarker==AT_DIR)
                    break;
                opt=setFileBeginOption(opt);
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor.linecount=dirs.file_count;
                cursor=setCursor(UP, 0, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                refreshDirInfo(dirs);
                break;
                break;
            case RENAME_FILE: //Rename a file
                if (cursor.winmarker==AT_DIR)
                    break;
                renameSelectedFile(currentDir, activelist[cursor.menuitem]);
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor.linecount=dirs.file_count;
                cursor=setCursor(UP, 0, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                break;
            case SELECTWITHEXTENSION: //Filter for a file extension
                opt=setFileFilter(opt);
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor.linecount=dirs.file_count;
                cursor=setCursor(UP, 0, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                refreshDirInfo(dirs);
                break;
            case OPEN_FILE:
                if(xdgFile(join(currentDir, activelist[cursor.menuitem]))==-1)
                {
                    strcpy(msg,"No application found for extension ");
                    strcat(msg,getFileExtension(activelist[cursor.menuitem]));
                    message(msg);
                }
                break;
            case DELETE_MARKED_FILE:
                cursor.menuitem-=deleteMarkedFile(&filemarker);
                free(filemarker);
                filemarker=NULL;
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor.linecount=dirs.file_count;
                cursor=setCursor(UP, cursor.menuitem, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                displayList(filemarker);
                refreshDirInfo(dirs);
                break;
            case DELETE_FILE:
                deleteFile(join(currentDir, activelist[cursor.menuitem]), true);
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor.linecount=dirs.file_count;
                cursor=setCursor(UP, --cursor.menuitem, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                refreshDirInfo(dirs);
                break;
            case MARK_FILE:
                markOneMoreFile(&filemarker, join(currentDir,activelist[cursor.menuitem]));
                displayList(filemarker);
                cursor=setCursor(DOWN, ++cursor.menuitem, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                break;
            case HIDDEN_FILE:
                opt.show_hidden = !opt.show_hidden;
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt); 
                if (cursor.winmarker==AT_FILE)
                    cursor.linecount=dirs.file_count;
                else
                    cursor.linecount=dirs.dir_count;
                cursor.menuitem=0;
                cursor.linemarker=0;
                cursor.arrowcounter=0;
                cursor=setCursor(UP, 0, cursor);
                drawmenu(filelist, filelist[0], winscrollable, 0);
                drawmenu(dirlist, dirlist[0], winmenu, 0);
                refreshDirInfo(dirs);
                //drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                break;
            case GOTO_DIR_NUMBER:
                cursor=setCursor(SAME, getNumber(winmenu),cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                break;
            case MOVE_PAGE_DOWN:
                cursor=setCursor(DOWN, cursor.menuitem+MENUMAX, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                break;
            case MOVE_PAGE_UP:
                cursor=setCursor(UP, cursor.menuitem-MENUMAX, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                break;
            case NEXT_DIR:
                if(cursor.winmarker==AT_DIR && dirs.dir_count==0)
                    break;
                cursor=setCursor(DOWN, ++cursor.menuitem, cursor);
                logger("\nCrsri ");
                logger(delimLong(cursor.menuitem));
                logger("\nActivelist_0 ");
                logger(activelist[0]);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                if(cursor.winmarker==1) 
                refreshFileInfo(currentDir, activelist[cursor.menuitem]);
                break;
            case PREVIOUS_DIR:
                cursor=setCursor(UP, --cursor.menuitem, cursor);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                if(cursor.winmarker==AT_FILE)
                    refreshFileInfo(currentDir, activelist[cursor.menuitem]);
                break;
            case '\t':
                wattrset(currentwin, A_NORMAL);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                cursor.winmarker=cursor.winmarker*-1;
                if(cursor.winmarker==AT_FILE)
                {
                    currentwin=winscrollable;
                    activelist=filelist;
                    cursor.linemax=FILEMAX;
                    cursor.linecount=dirs.file_count;
                    cursor.diritemold=cursor.menuitem;
                    cursor.menuitem=cursor.fileitemold;
                    cursor.arrowcounterdir=cursor.arrowcounter;
                    cursor.arrowcounter=cursor.arrowcounterfile;
                    cursor.linemarkerdir=cursor.linemarker;
                    cursor.linemarker=cursor.linemarkerfile;
                    werase(winfileinfo);
                    wrefresh(winfileinfo);
                }
                else
                {
                    currentwin=winmenu;
                    activelist=dirlist;
                    cursor.linemax=MENUMAX;
                    cursor.linecount=dirs.dir_count;
                    cursor.fileitemold=cursor.menuitem;
                    cursor.menuitem=cursor.diritemold;
                    cursor.arrowcounterfile=cursor.arrowcounter;
                    cursor.arrowcounter=cursor.arrowcounterdir;
                    cursor.linemarkerfile=cursor.linemarker;
                    cursor.linemarker=cursor.linemarkerdir;
                }
                wattrset(currentwin, A_BOLD);
                drawmenu(activelist, activelist[cursor.menuitem], currentwin, cursor.linemarker);
                break;
            case VIEW_FILES:
                if(strcmp(dirlist[cursor.menuitem],"..")==0 || strcmp(dirlist[cursor.menuitem],".")==0)
                    break;
                cursor.linemarker=0;
                cursor.arrowcounter=0;
                if(strcmp(dirlist[cursor.menuitem],"..")!=0)
                    strncat(currentDir,dirlist[cursor.menuitem],strlen(dirlist[cursor.menuitem]));
                else
                    split('/', currentDir);
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor.linecount=dirs.dir_count;
                logger("\ncursor.linecount");
                logger(delimLong(cursor.linecount));
                drawmenu(dirlist, currentDir, winmenu, cursor.linemarker);
                cursor.linemarker=0;
                drawmenu(filelist, "---", winscrollable, cursor.linemarker);
                refreshDirInfo(dirs);
                cursor.menuitem = 0;
                break;
            case PARENT_DIR:
                if (cursor.winmarker==AT_FILE)
                    break;
                currentwin=winmenu;
                split('/', currentDir);
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor.linecount=dirs.dir_count;
                drawmenu(dirlist, currentDir, winmenu, cursor.linemarker);
                cursor.menuitem = 0;
                cursor.linemarker=0;
                cursor.arrowcounter=0;
                drawmenu(filelist, "---", winscrollable, cursor.linemarker);
                refreshDirInfo(dirs);
                break;
            case GOTO_HOME_DIR:
                currentwin=winmenu;
                strcpy(currentDir, GetUserDir());
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor=setCursor(UP, cursor.menuitem, cursor);
                drawmenu(filelist, filelist[0], winscrollable, 0);
                drawmenu(dirlist, dirlist[0], winmenu, 0);
                refreshDirInfo(dirs);
                break;
            case GOTO_ROOT_DIR:
                currentwin=winmenu;
                strcpy(currentDir, ROOT);
                logger(currentDir);
                dirs=DoDirectoryList(currentDir, dirlist, filelist, opt);
                cursor=setCursor(UP, cursor.menuitem, cursor);
                drawmenu(filelist, filelist[0], winscrollable, 0);
                drawmenu(dirlist, dirlist[0], winmenu, 0);
                refreshDirInfo(dirs);
                break;

        }
        werase(winheader);
        mvwprintw(winheader,0,0,">>%-30s    ",currentDir);
        wrefresh(winheader);
	}  while(key != 'q');
	echo();	
	endwin();
	return 0;
}
