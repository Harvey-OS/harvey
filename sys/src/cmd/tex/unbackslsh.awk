#! /usr/bin/awk -f
# The relevant lines look like `  \'.
/^ *\\$/{
	  if (!backslashed)
	    {
	      #print "not backslashed -- setting to empty backslash";
	      backslashed = $0;
	    }
	  next;
	}
/\\$/	{
          if (backslashed)
            {
              #print "already backslashed";
              print backslashed;
            }
            
          backslashed = $0;
          #print "setting bs to " backslashed;
          next;
        }

/^ *$/	{ 
          if (backslashed)
	    {
	      #print "spaces only -- prev was backslashed";
	      print substr (backslashed, 1, length (backslashed) - 1);
  	      backslashed = "";
	    }
	  else
	    {
	      #print "spaces only -- prev not backslashed";
	      print;
	    }
	  next;
	}

	{ 
          if (backslashed)
	    {
	      #print "normal line -- prev backslashed";
  	      print backslashed;
  	      backslashed = "";
	    }
	  print
	}
