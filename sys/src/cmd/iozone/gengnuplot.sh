#!/bin/sh
#
# Copyright (c) 2001 Yves Rougy Yves@Rougy.net
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



cp $1 iozone_gen_out
file_name=iozone_gen_out
#set -x

write_gnuplot_file() {
	echo \#test : $query
	case $query in
  		(write) awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $3  }' < $file_name ;;
  		(rewrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $4  }' < $file_name ;;
  		(read)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $5  }' < $file_name ;;
  		(reread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $6  }' < $file_name ;;
  		(randread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $7  }' < $file_name ;;
  		(randwrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $8  }' < $file_name ;;
  		(bkwdread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $9  }' < $file_name ;;
  		(recrewrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $10  }' < $file_name ;;
  		(strideread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $11  }' < $file_name ;;
  		(fwrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $12  }' < $file_name ;;
  		(frewrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $13  }' < $file_name ;;
  		(fread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $14  }' < $file_name ;;
  		(freread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $15  }' < $file_name ;;
		(*)  echo "Usage : gengnuplot.sh <filename> <test>" >> /dev/stderr ; 
       		     echo "filename is the output of iozone -a" >> /dev/stderr ;
       		     echo "test is one of write rewrite read reread randread randwrite bkwdread recrewrite strideread fwrite frewrite fread freread" >> /dev/stderr ;;
	esac }

#filename=$1
filename=iozone_gen_out
query=$2
if (! [ -e $query ] ) ; then mkdir $query; fi
if ( [ $# -eq 2 ] ) ; 
	then 
		write_gnuplot_file > $query/`basename $file_name.gnuplot`
	else
		echo "Usage : gengnuplot.sh <filename> <test>" 2>&1
		echo "filename is the output of iozone -a" 2>&1
		echo "test is one of write rewrite read reread randread randwrite bkwdread recrewrite strideread fwrite frewrite fread freread" 2>&1
fi
