	: again
	/\.$/{
		N
		s/\.\n/\
./
		t again
	}
	s/\.hh/.hhfield/g
	s/\.lh/.lhfield/g
