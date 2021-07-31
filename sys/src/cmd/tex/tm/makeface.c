#include <stream.h>
#include <fstream.h>
unsigned int temp;
unsigned int inface[48][3];
unsigned short outface[48][6];
char ch;

main(int argc, char* argv[])
{
  fstream facefile;
  fstream mapfile;

  if(argc>=1){
    facefile.open(argv[1],input);
    if(!facefile)
      cerr<<"cannot open input  file\n";
  }
  else {cerr<<"must specify an input file\n";
        cerr<<"makeface input_file output_file\n";
      }
//  else facefile=cin;

  if(argc>=2) {
    mapfile.open(argv[2],output);
    if(!mapfile)
      cerr<<"cannot open output file\n";
  }
    else {cerr<<"must specify an output file\n";
          cerr<<"makeface input_file output_file\n";
	}
//  else mapfile=cout;

  for(int i=0;i<48;i++){
    for(int j=0;j<3;j++){
      facefile>>resetiosflags(ios::basefield)>>temp>>ch;
      if(facefile.eof()||facefile.bad()){cerr<<"bad read";}

      unsigned short temp1=0;
      short mask;
      for(int bit=0;bit<16;bit++){
	temp1>>=1;            // shift down; first time nothing
	mask = temp&0x8000;   // get top bit
	temp<<=1;             // shift 
	temp1|=mask;          // put bit in top of temp1
      }
      outface[i][2*j+1]= (temp1&0xff00)>>8;
      outface[i][2*j]= (temp1&0x00ff);
    }
  }

  mapfile<<"#define face_width 48\n";
  mapfile<<"#define face_height 48\n";
  mapfile<<"static char face_bits[] = {\n";
  mapfile.fill('0');
  for(i=0;i<48;i++){
    for(int j=0;j<6;j++){
      mapfile<<"0x"<<setw(2)<<hex<<outface[i][j]<<", ";
    }
    mapfile << "\n";
  }
  mapfile<<"};\n";
}


