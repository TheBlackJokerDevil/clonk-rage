/*
**  bmp2png --- conversion from (Windows or OS/2 style) BMP to PNG
**  png2bmp --- conversion from PNG to (Windows style) BMP
**
**  Copyright (C) 1999-2000 MIYASAKA Masaru
**
**  For conditions of distribution and use,
**  see copyright notice in common.h.
*/

#include "common.h"

#if defined(__DJGPP__)	/* DJGPP V.2 */
#include <crt0.h>
int _crt0_startup_flags = _CRT0_FLAG_DISALLOW_RESPONSE_FILES;
unsigned short _djstat_flags =		/* for stat() */
	_STAT_INODE|_STAT_EXEC_EXT|_STAT_EXEC_MAGIC|_STAT_DIRSIZE|_STAT_ROOT_TIME;
#endif


/* -----------------------------------------------------------------------
**		�i�s�󋵁E�G���[���b�Z�[�W�\��
*/

#define LINE_LEN	79
#define STAT_LEN	22
#define DOTS_MAX	(LINE_LEN-STAT_LEN-1)

static char status_msg[320];
static int  scaleflag = 0;
static int  maxdots   = 0;
static int  currdots  = -1;

int quietmode = 1;


static void print_status( void )
{
	fprintf( stderr, "\r%-*.*s ", STAT_LEN, STAT_LEN, status_msg );
	fflush( stderr );
	currdots = 0;
}

static void put_dots( int dotchar, int num )
{
	int m,n;

	if( num > DOTS_MAX ) num = DOTS_MAX;
	if( currdots==-1 ) print_status();

	for( m=n=(num-currdots) ; --n>=0 ; ){
		fputc( dotchar, stderr );
	}
	if( m>0 ){
		currdots = num;
		fflush( stderr );
	}
}

static void print_scale( void )
{
	print_status();
	put_dots( '.', maxdots );
	print_status();
	scaleflag = 1;
}

static void init_progress( int max )
{
	if( quietmode ) return;

	maxdots = max;
	print_scale();
}

static void update_progress( int num )
{
	if( quietmode ) return;

	if( !scaleflag ) print_scale();
	put_dots( 'o', num );
}

static void clear_line( void )
{
	if( quietmode ) return;

	fprintf( stderr, "\r%*c\r", LINE_LEN, ' ' );
	scaleflag = 0;
	currdots = -1;
}

void xprintf( const char *fmt, ... )
{
	va_list ap;

	clear_line();

	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	fflush( stderr );
	va_end( ap );
}

void set_status( const char *fmt, ... )
{
	va_list ap;

	if( quietmode ) return;

	va_start( ap, fmt );
	vsprintf( status_msg, fmt, ap );
	va_end( ap );

	print_status();
}

void feed_line( void )
{
	if( quietmode ) return;

	fputc( '\n', stderr );
	fflush( stderr );
	scaleflag = 0;
	currdots = -1;
}


/* -----------------------------------------------------------------------
**		Libpng �i�s�󋵕\��
*/

/* --------------------------------------------------- *

  PNG �̃C���^�[���X�摜�`�� "Adam7" �̉摜�p�����[�^

  �p�X   ��  ����  �J�n�_ �ʐϔ� �i�s��
    0   1/8   1/8   (0,0)   1/64  1/64
    1   1/8   1/8   (4,0)   1/64  1/32
    2   1/4   1/8   (0,4)   1/32  1/16
    3   1/4   1/4   (2,0)   1/16  1/8
    4   1/2   1/4   (0,2)   1/8   1/4
    5   1/2   1/2   (1,0)   1/4   1/2
    6    1    1/2   (0,1)   1/2    1


  Adam7 �̎��̐i�s�󋵎Z�o�@�F

  (width/8) * 1 �̃s�N�Z���O���[�v���P�u���b�N�ƍl���A
  ���̃u���b�N��ʎZ�ł����o�͂������Ői�s�󋵂�
  �Z�o����B�Ⴆ�΁Apass0 �̎��́A��s�o�͂����
  �P�u���b�N�Apass4 �̎��́A��s�łS�u���b�N�A�ƂȂ�B
  �C���^�[���X�ɂ��Ȃ������ꍇ�͈�s=�W�u���b�N��
  ���ɂȂ�̂ŁA���̃u���b�N�����W�Ŋ���ƁA
  �C���^�[���X�ɂ��Ȃ������ꍇ�Ɋ��Z���ĉ��s������
  �o�͂�����������B

 * --------------------------------------------------- */

static png_uint_32 counter;
static png_uint_32 maxcount;
static png_uint_32 step;
static int         currpass;
static int         scaledots;

/*
**		Adam7 �̎��̑��J�E���g�������߂�
*/
static png_uint_32
maxcount_adam7( png_uint_32 width, png_uint_32 height )
{
	png_uint_32 c = 0;

	if(    1    ) c += ((height-0+7)/8)*1;		/* Pass 0 */
	if( width>4 ) c += ((height-0+7)/8)*1;		/* Pass 1 */
	if(    1    ) c += ((height-4+7)/8)*2;		/* Pass 2 */
	if( width>2 ) c += ((height-0+3)/4)*2;		/* Pass 3 */
	if(    1    ) c += ((height-2+3)/4)*4;		/* Pass 4 */
	if( width>1 ) c += ((height-0+1)/2)*4;		/* Pass 5 */
	if(    1    ) c += ((height-1+1)/2)*8;		/* Pass 6 */

	return c;
}


/*
**		�i�s�󋵕\���̏����ݒ�
*/
void init_row_callback( png_structp png_ptr, png_uint_32 width,
                        png_uint_32 height )
{
	enum { W=1024,H=768 };

	if( png_set_interlace_handling(png_ptr)==7 ){
		currpass = -1;		/* interlace */
		step     = 0;
		maxcount = maxcount_adam7( width, height );
	}else{
		currpass = 0;		/* non-interlace */
		step     = 8;
		maxcount = 8*height;
	}
	if( height > ((png_uint_32)W*H)/width ){
		scaledots = DOTS_MAX;
	}else{
		scaledots = (DOTS_MAX * width * height + (W*H-1)) / (W*H);
	}
	counter = 0;
	init_progress( scaledots );
}


/*
**		�i�s�󋵂̕\��
*/
void row_callback( png_structp png_ptr, png_uint_32 row, int pass )
{
	if( row==0 ) pass--;
		/*
		**	Libpng �̃o�O(?)�΍�(^^;�B�m���C���^�[���[�X�t�@�C���̏ꍇ�A
		**	row �� 1 ����n�܂��� height �܂ŕω����邪�A�C���^�[���[�X
		**	�t�@�C���̏ꍇ�́Arow=height,pass=n �ƂȂ�ׂ��Ƃ���
		**	row=0,pass=n+1 �Ƃ����l��Ԃ����߁B
		*/

	if( currpass!=pass ){
		currpass = pass;
		step = 1<<(pass>>1);
	}
	counter += step;
	update_progress( scaledots * counter / maxcount );
}


/* -----------------------------------------------------------------------
**		Libpng �G���[�\��
*/

/*
**		Libpng �p�G���[�\���֐�
*/
void png_my_error( png_structp png_ptr, png_const_charp message )
{
	xprintf( "ERROR(libpng): %s - %s\n", message,
	               (char *)png_get_error_ptr(png_ptr) );
	longjmp( png_ptr->jmpbuf, 1 );
}


/*
**		Libpng �p�x���\���֐�
*/
void png_my_warning( png_structp png_ptr, png_const_charp message )
{
	xprintf( "WARNING(libpng): %s - %s\n", message,
	               (char *)png_get_error_ptr(png_ptr) );
}


/* -----------------------------------------------------------------------
**		�C���[�W�o�b�t�@�������Ǘ�
*/

/*
**		�C���[�W�o�b�t�@�p�������̈���m�ۂ���
*/
BOOL imgalloc( IMAGE *img )
{
	BYTE *bp,**rp;
	DWORD rb;
	LONG n;

	if( img->palnum>0 ){
		img->palette = (png_color_struct*) malloc( (size_t)img->palnum * sizeof(PALETTE) );
		if( img->palette==NULL ) return FALSE;
	}else{
		img->palette = NULL;
	}
	img->rowbytes = ((DWORD)img->width * img->pixdepth + 31) / 32 * 4;
	img->imgbytes = img->rowbytes * img->height;
	img->rowptr   = (unsigned char**) malloc( (size_t)img->height * sizeof(BYTE *) );
	img->bmpbits  = (unsigned char*) malloc( (size_t)img->imgbytes );

	if( img->rowptr==NULL || img->bmpbits==NULL ){
		imgfree( img );
		return FALSE;
	}

	n  = img->height;
	rp = img->rowptr;
	rb = img->rowbytes - sizeof(DWORD);

	if( img->topdown ){
		bp = img->bmpbits;
		while( --n>=0 ){
			*(rp++) = bp;
			bp += rb;
			*((DWORD *)bp) = 0;
			bp += sizeof(DWORD);
		}
	}else{
		bp = img->bmpbits + img->imgbytes;
		while( --n>=0 ){
			bp -= sizeof(DWORD);
			*((DWORD *)bp) = 0;
			bp -= rb;
			*(rp++) = bp;
		}
	}

	return TRUE;
}


/*
**		�C���[�W�o�b�t�@�p�������̈���������
*/
void imgfree( IMAGE *img ) 
{
	if( img->palette!=NULL ) free( img->palette );
	if( img->rowptr !=NULL ) free( img->rowptr  );
	if( img->bmpbits!=NULL ) free( img->bmpbits );
}


/* -----------------------------------------------------------------------
**		�R�}���h���C�������̏���
*/

#define isoption(p)	(isoptchar(*(p)) && *((p)+1)!='\0')

/*
**	�R�}���h���C��������ǂ�
*/
int parsearg( int *opt, char **arg, int argc, char **argv, char *aopts )
{
	static int   agi = 1;
	static char *agp = NULL;
	char *p;
	int c,i;

	if( agp!=NULL && *agp=='\0' ){
		agp = NULL;
		agi++;
	}
	if( agi>=argc ) return 0;		/* end */

	if( p=argv[agi], agp==NULL && !isoption(p) ){
		/* non-option element */
		c = 0;
		agi++;
	}else{
		if( agp==NULL ) agp = p+1;
		if( c=*agp, strchr(aopts,c)!=NULL ){
			/* option with an argument */
			if( p=agp+1, *p!='\0' ) ;
			else if( i=agi+1, p=argv[i], i<argc && !isoption(p) ) agi = i;
			else p = NULL;
			agp = NULL;
			agi++;
		}else{
			/* option without an argument */
			p = NULL;
			agp++;
		}
	}
	*opt = c;
	*arg = p;

	return 1;
}


/*
**	���ϐ��Ŏw�肳��Ă���I�v�V������ argc, argv �ɕ�������
*/
char **envargv( int *argcp, char ***argvp, const char *envn )
{
	int argc,nagc,envc,i;
	char **argv,**nagv,*envs,*ep;

	ep = getenv( envn );
	if( ep==NULL || ep[0]=='\0' ) return NULL;

	envs = (char*) malloc( strlen(ep)+1 );
	if( envs==NULL ) return NULL;
	strcpy( envs, ep );

	envc = tokenize( envs, envs );
	if( envc==0 ){ free(envs); return NULL; }

	argc = *argcp;
	argv = *argvp;
	nagv = (char**) malloc ( (argc+envc+1) * sizeof(char *) );
	if( nagv==NULL ){ free(envs); return NULL; }

	nagc = 1;
	nagv[0] = argv[0];

	for( i=0 ; i<envc ; i++ ){
		nagv[nagc++] = envs;
		while( *(envs++)!='\0' );
	}
	for( i=1 ; i<argc ; i++ ){
		nagv[nagc++] = argv[i];
	}
	nagv[nagc] = NULL;

	*argcp = nagc;
	*argvp = nagv;

	return argv;
}


/*
**	��������󔒕���(�X�y�[�X/�����^�u/���s)�̏��ŋ�؂�(�N�I�[�g�����t��)
**	��؂�ꂽ����������̐���Ԃ�
*/
int tokenize( char *buf, const char *str )
{
	char c,instr=0,inquote=0;
	int n,num=0;

	while( (c=*(str++))!='\0' ){
		if( !inquote && (c==' ' || c=='\t' || c=='\n' || c=='\r') ){
			if( instr ){
				instr = 0;
				*(buf++) = '\0';
			}
		}else{
			if( !instr ){
				instr = 1;
				num++;
			}
			switch(c){
			case '\\':
				/*
				**	Escaping of `"' is the same as
				**	command-line parsing of MS-VC++.
				**
				**	ex.) "     ->      quote
				**	     \"    ->      "
				**	     \\"   -> \  + quote
				**	     \\\"  -> \  + "
				**	     \\\\" -> \\ + quote
				**	     ...
				**	     \\\\\ -> \\\\\
				*/
				for( n=1 ; (c=*str)=='\\' ; str++,n++ );
				if( c=='"' ){
					while( (n-=2)>=0 )
						*(buf++) = '\\';
					if( n==-1 ){
						*(buf++) = '"';
						str++;
					}
				}else{
					while( (--n)>=0 )
						*(buf++) = '\\';
				}
				break;

			case '"':
				inquote ^= 1;
				break;

			default:
				*(buf++) = c;
			}
		}
	}
	if( instr ) *buf = '\0';

	return num;
}


/* -----------------------------------------------------------------------
**		�t�@�C���Ɋւ���G�p����
*/

/*
**	�����K�w�̃f�B���N�g������x�ɍ쐬����
*/
int makedir( const char *path )
{
	char dir[FILENAME_MAX];
	char c,*p,*q;
	int r = 0;

	switch( filetype(path) ){
		case 1: /* errno = EEXIST; */ return -1;
		case 2:                       return  0;	/* nothing to do */
	}
	strcpy( dir, path );
	for( p=path_skiproot(dir) ; q=path_nextslash(p),(c=*q)!='\0' ; p=q+1 ){
		*q = '\0';
		r = MKDIR( dir, 0777 );
		*q = c;
	}
	if( q!=p )
		r = MKDIR( dir, 0777 );

	return r;
}


/*
**	�����̓����t�@�C�����o�b�N�A�b�v(���l�[��)����
*/
int renbak( const char *path )
{
	char bak[FILENAME_MAX];
	char *ext;
	int i;

	if( filetype(path)==0 ) return 1;

	strcpy( bak, path );
#ifdef MSDOS
	ext = suffix( bak );
#else
	ext = bak + strlen( bak );
#endif
	strcpy( ext, ".bak" );
	i = 0;
	do{
		if( filetype(bak)==0 && rename(path,bak)==0 ) return 1;
		sprintf( ext, ".%03d", i );
	}while( (i++)<=999 );

	return 0;
}


/*
**	�w�肳�ꂽ���O�̃t�@�C��/�f�B���N�g�������݂��邩�ǂ����A
**	���݂���ꍇ�͂��ꂪ�t�@�C�����f�B���N�g�����𒲂ׂ�B
**
**	return  = 0:not exist, 1:file, 2:directory
*/
int filetype( const char *name )
{
	char fn[FILENAME_MAX];
	struct stat sbuf;
	int r = 0;

	strcpy( fn, name );
	delslash( fn );

	if( stat(fn,&sbuf)==0 )
		r = ((sbuf.st_mode & S_IFMT)==S_IFDIR)? 2 : 1;

	return r;
}


/*
**	�t�@�C���̃^�C���X�^���v���R�s�[����
*/
int cpyftime( const char *src, const char *dest )
{
	struct stat    sbuf;
	struct utimbuf ubuf;

	if( stat(src,&sbuf)!=0 ) return -1;

	ubuf.actime  = sbuf.st_atime;
	ubuf.modtime = sbuf.st_mtime;

	return utime( dest, &ubuf );
}


/* -----------------------------------------------------------------------
**   �t�@�C�����E�p�X��������̏���
*/

/*
**	path �̊g���q�����ւ̃|�C���^��Ԃ�
**	ex.) c:\dosuty\log\test.exe -> .exe
**	ex.) c:\dosuty\log\test.tar.gz -> .gz
*/
char *suffix( const char *path )
{
	char c,*p,*q,*r;

	for( r=q=p=bname(path) ; (c=*p)!='\0' ; p++ )
									if( c=='.' ) q=p;
		/*
		 * �擪�� . �͊g���q�Ƃ݂͂Ȃ��Ȃ�
		 * (UNIX�ł̕s�������t�@�C��)
		 */
	if( q==r ) q=p;

	return q;
}


/*
**	path �̃t�@�C���������ւ̃|�C���^��Ԃ�
**	ex.) c:\dos\format.exe -> format.exe
*/
char *bname( const char *path )
{
	const char *p,*q;

	for( p=path_skiproot(path) ; *(q=path_nextslash(p))!='\0' ; p=q+1 );

	return (char *)p;
}


/*
**	path �̍Ō��(�p�X��)��؂蕶����t���A��Ƀt�@�C������P���A��
**	�ł���`�ɂ���B"c:\", "\", "c:", "" �Ȃǂ̏ꍇ�͉������Ȃ��B
**	ex.) c:\dos -> c:\dos\
*/
char *addslash( char *path )
{
	char *p,*q;

	for( p=path_skiproot(path) ; *(q=path_nextslash(p))!='\0' ; p=q+1 );
	/*
	**	s = path_skiproot( path );
	**	if( q==s && q==p ) - s ���󕶎���
	**	if( q!=s && q==p ) - s ����؂蕶���ŏI����Ă���
	**	if( q!=s && q!=p ) - s ����؂蕶���ŏI����Ă��Ȃ�
	*/
	if( q!=p ){ *(q++)=PDELIM_PRIMARY; *(q)='\0'; }

	return path;
}


/*
**	path �̍Ō��(�p�X��)��؂蕶�������A�f�B���N�g�����ŏI���
**	�`�ɂ���B"c:\", "\", "c:", "" �Ȃǂ̏ꍇ�� '.' ��t��������B
**	ex.) c:\dos\ -> c:\dos
**	     c:\ -> c:\.
*/
char *delslash( char *path )
{
	char *p,*q,*s;

	for( p=s=path_skiproot(path) ; *(q=path_nextslash(p))!='\0' ; p=q+1 );
	/*
	**	if( q==s && q==p ) - s ���󕶎���
	**	if( q!=s && q==p ) - s ����؂蕶���ŏI����Ă���
	**	if( q!=s && q!=p ) - s ����؂蕶���ŏI����Ă��Ȃ�
	*/
	if( q==s ){ *(q++)='.'; *(q)='\0'; }
	else if( q==p ) *(--q)='\0';

	return path;
}


/*
**	path �̐擪�Ɍ���郋�[�g�w�蕔(c:\)���X�L�b�v����
*/
char *path_skiproot( const char *path )
{
#ifdef PDELIM_DRIVESPEC
	if( isalpha(path[0]) && path[1]==PDELIM_DRIVESPEC ) path += 2;
#endif
	if( IsPathDelim(path[0]) ) path++;
	return (char *)path;
}


/*
**	path �̒���(�p�X��)��؂蕶�����ŏ��Ɍ��ꂽ�ʒu�ւ̃|�C���^��Ԃ�
*/
char *path_nextslash( const char *path )
{
#ifdef DBCS_PATH
	int f=0;
#endif
	char c;

	for( ; (c=*path)!='\0' ; path++ ){
#ifdef DBCS_PATH
		if( f==1 || IsDBCSLead(c) ){
			f^=1;  continue;
		}
#endif
		if( IsPathDelim(c) ) break;
	}
	return (char *)path;
}

#ifdef WIN32_LFN

/*
**	MS-DOS �`���̊g���q���ǂ����𒲂ׂ�
*/
int is_dos_extension( const char *str )
{
	const char *p;
	char c;

	if( *str!='.' || *(++str)=='\0' ) return 0;

	for( p=str ; (c=*p)!='\0' ; p++ )
			if( islower(c) ) return 0;

	return ((p-str)<=3);
}

#endif /* WIN32_LFN */

