c     .. display all of the Hershey font data
c
c     .. By James Hurt when with
c     ..    Deere and Company
c     ..    John Deere Road
c     ..    Moline, IL 61265
c
c     .. Author now with Cognition, Inc.
c     ..                 900 Technology Park Drive
c     ..                 Billerica, MA 01821
c
c     .. graphics subroutines
c     .. identy - initialize graphics
c     .. vwport - set where to display image on screen
c     ..        - full screen is 0.0 to 100.0 in vertical (y) direction
c     ..        -                0.0 to ???.? in horizontal (x) direction
c     ..        - origin is lower left corner of screen
c     .. window - set window limits in world coordinates
c     .. newpag - if action left to be take on existing screen, take it
c     ..        - then take actions to start with a blank screen
c     .. jnumbr - display an integer (code included)
c     .. move   - set current cursor position to (x,y)
c     .. draw   - draw from current cursor position to (x,y)
c     ..        - then set current cursor position to (x,y)
c     ..        - The point (x,y) is always in world coordinates
c     .. skip   - Make the next draw really be a move
c     .. waitcr - finish all graphics actions then let the user look at
c     ..        - the image.  User signals (usually by pressing RETURN)
c     ..        - when it is safe to continue.
c     .. grstop - finish all graphics routines (no more graphics to follow)
c
      external identy,vwport,window,newpag,jnumbr,move ,draw ,skip,
     x         waitcr,grstop
c     .. local variables
      real deltac, deltar, colmax
      parameter (deltac = 6.25, deltar = 6.25, colmax = 100.0)
c     .. font data file name
      character*80 name
c     .. font data
      character*1 line(2,256)
c     .. co-ordinates
      real x,y,col,row
c     .. which data point and which character
      integer ipnt,ich,nch,i
      intrinsic ichar
cexecutable code begins
c     .. file unit number
      kfile=1
c     .. get hershey file name
      write(*,'(a)') ' packed hershey font file name'
      read(*,'(a)') name
      open(unit=kfile,file=name,status='old')
c     .. initialize graphics
      call identy
c     .. want square picture for each character
c     .. Note: most but not all Hershey font characters fit inside this window
      call window(-15.0, 15.0,-15.0, 15.0)
c     .. loop per screen
5     continue
c     .. start with a clean sheet
      call newpag
c     .. where to display this character
      col = 0.0
      row = 100.0
c     .. loop per character
10    continue
c     .. read character number and data
      read(unit=kfile,'(i5,i3,64a1/(72a1))',end=90) ich,nch,
     x     (line(1,i),line(2,i),i=1,nch)
c     .. select view port (place character on screen)
      call vwport(col,col+deltac,row-deltar,row)
c     .. identify character
      call jnumbr(ich,4,-15.0,9.0,5.0)
c     .. draw character limits
c     .. Note: this data can be used for proportional spacing
      x=ichar(line(1,1))-ichar('R')
      y=ichar(line(2,1))-ichar('R')
      call move(x,-10.0)
      call draw(x,10.0)
      call move(y,-10.0)
      call draw(y,10.0)
c     .. first data point is a move
      call skip
c     .. loop per line of data
      do 20 ipnt = 2, nch
c     .. process vector number ipnt
      if(line(1,ipnt).eq.' ') then
c        .. next data point is a move
         call skip
      else
c        .. draw (or move) to this data point
         x=ichar(line(1,ipnt))-ichar('R')
         y=ichar(line(2,ipnt))-ichar('R')
c        .. Note that Hershey Font data is in TV coordinate system
         call draw(x,-y)
      endif
20    continue
c     .. end of this character
      col = col + deltac
      if( col .lt. colmax ) go to 10
      col = 0.0
      row = row - deltar
      if( row .ge. deltar ) go to 10
      call waitcr
      go to 5
90    continue
      call waitcr
c     .. all done
      call grstop
      end
      subroutine jnumbr( number, iwidth, x0, y0, height )
      integer number, iwidth
      real x0, y0, height
c     .. draw one of the decimal digits
c     .. number = the integer to be displayed
c     .. iwidth = the number of characters
c     .. (x0, y0) = the lower left corner
c     .. height = height of the characters
c
c
c     .. By James Hurt when with
c     ..    Deere and Company
c     ..    John Deere Road
c     ..    Moline, IL 61265
c
c     .. Author now with Cognition, Inc.
c     ..                 900 Technology Park Drive
c     ..                 Billerica, MA 01821
c
c     .. graphics (graphics) routines called
      external skip,draw
c     .. local variables used
      integer ipnt, ipos, ival, idigit
      real x, y, scale
      real xleft, ylower
c     .. character data for the ten decimal digit characters
c     .. data extracted from one of the Hershey fonts
      integer start(0:10), power(0:9)
      character*1 line(2,104)
      data power/ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
     x 100000000, 1000000000 /
      data start/0,11,14,22,36,42,55,68,73,91,104/
c 0:poly(4 9,2 8,1 6,1 3,2 1,4 0,6 1,7 3,7 6,6 8,4 9)
c 1:poly(2 7,4 9,4 0)
c 2:poly(1 8,3 9,5 9,7 8,7 6,6 4,1 0,7 0)
c 3:poly(1 8,3 9,5 9,7 8,7 6,5 5)
c   poly(4 5,5 5,7 4,7 1,5 0,3 0,1 1)
c 4:poly(5 9,5 0)
c   poly(5 9,0 3,8 3)
c 5:poly(2 9,1 5,3 6,4 6,6 5,7 3,6 1,4 0,3 0,1 1)
c   poly(2 9,6 9)
c 6:poly(6 9,4 9,2 8,1 6,1 3,2 1,4 0,6 1,7 3,6 5,4 6,2 5,1 3)
c 7:poly(7 9,3 0)
c   poly(1 9,7 9)
c 8:poly(3 9,1 8,1 6,3 5,5 5,7 6,7 8,5 9,3 9)
c   poly(3 5,1 4,1 1,3 0,5 0,7 1,7 4,5 5)
c 9:poly(7 6,6 4,4 3,2 4,1 6,2 8,4 9,6 8,7 6,7 3,6 1,4 0,2 0)
c
      data line/'R','M','P','N','O','P','O','S','P','U','R','V','T','U',
     A'U','S','U','P','T','N','R','M','P','O','R','M','R
     B','V','O','N','Q','M','S','M','U','N','U','P','T','R','O',
     C'V','U','V','O','N','Q','M','S','M','U','N','U','P','S','Q
     D',' ','R','R','Q','S','Q','U','R','U','U','S','V','Q','V','O','U',
     E'S','M','S','V',' ','R','S','M','N','S','V','S','P
     F','M','O','Q','Q','P','R','P','T','Q','U','S','T','U','R','V','Q',
     G'V','O','U',' ','R','P','M','T','M','T','M','R','M','P','N
     H','O','P','O','S','P','U','R','V','T','U','U','S','T','Q','R','P',
     I'P','Q','O','S','U','M','Q','V',' ','R','O','M','U','M',
     J'Q','M','O','N','O','P','Q','Q','S','Q','U','P','U','N','S',
     K'M','Q','M',' ','R','Q','Q','O','R','O','U','Q','V','S','V','U','U
     L','U','R','S','Q','U','P','T','R','R','S','P','R','O','P',
     M'P','N','R','M','T','N','U','P','U','S','T','U','R','V','P','V'/
c     .. compute scale factor and lower left of first digit
      scale = height/10.0
      xleft = x0
      ylower = y0
      ival = number
c     .. loop for each position
      do 30 ipos = iwidth,1,-1
         idigit = mod( ival/power(ipos-1), 10 )
c        .. first data point is a move
         call skip
c        .. loop over data for this digit
         do 20 ipnt=start(idigit)+1,start(idigit+1)
            if(line(1,ipnt).eq.' ') then
c              .. next data point is a move
               call skip
            else
c              .. draw (or move) to this data point
               x=ichar(line(1,ipnt))-ichar('N')
               y=ichar(line(2,ipnt))-ichar('V')
               call draw(xleft+scale*x,ylower-scale*y)
            endif
20       continue
c        .. move for next digit
         xleft = xleft + height
30    continue
      end

