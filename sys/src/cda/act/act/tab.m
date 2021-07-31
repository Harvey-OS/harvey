.o enx@b frame xin shift8
 0:6 0:1
.o eny@b frame xin shift8
 0:4 0:1 2:2
.o xin@b smin prev
 1:3 2:3
.o tx00@b tz00
 1:1
.o tx01@b tz01
 1:1
.o tx02@b tz02
 1:1
.o tx03@b tz03
 1:1
.o tx04@b tz04
 1:1
.o tx05@b tz05
 1:1
.o tx06@b tz06
 1:1
.o tx07@b tz07
 1:1
.o tx08@b tz08
 1:1
.o tx09@b tz09
 1:1
.o tx10@b tz10
 1:1
.o ty00@b s0 s1 t13 t01
 7:7 8:9 8:10
.o ty01@b s0 s1 t14 t02
 7:7 8:9 8:10
.o ty02@b s0 s1 t15 t03
 7:7 8:9 8:10
.o ty03@b s0 s1 t16 t04
 7:7 8:9 8:10
.o ty04@b s0 s1 t17 t05
 7:7 8:9 8:10
.o ty05@b s0 s1 t18 t06
 7:7 8:9 8:10
.o ty06@b s0 s1 t19 t07
 7:7 8:9 8:10
.o ty07@b s0 s1 t20 t08
 7:7 8:9 8:10
.o ty08@b s0 s1 t21 t09
 7:7 8:9 8:10
.o ty09@b s0 s1 t22 t10
 7:7 8:9 8:10
.o ty10@b s0 s1 t23 t11
 7:7 8:9 8:10
.o tz00@b ty00
 1:1
.o tz01@b ty01
 1:1
.o tz02@b ty02
 1:1
.o tz03@b ty03
 1:1
.o tz04@b ty04
 1:1
.o tz05@b ty05
 1:1
.o tz06@b ty06
 1:1
.o tz07@b ty07
 1:1
.o tz08@b ty08
 1:1
.o tz09@b ty09
 1:1
.o tz10@b ty10
 1:1
.o gxn@b r1 r0 gxn s1 s0
 4:28 2:2 1:1
.o gyn@b r1 r0 gyn s1 s0
 4:20 4:12 3:3
.o gzn@b r1 gzn s1
 2:6 1:1
.o t00@b t00 tc00
 1:3 2:3
.o t01@b t01 tc01
 1:3 2:3
.o t02@b t02 tc02
 1:3 2:3
.o t03@b t03 tc03
 1:3 2:3
.o t04@b t04 tc04
 1:3 2:3
.o t05@b t05 tc05
 1:3 2:3
.o t06@b t06 tc06
 1:3 2:3
.o t07@b t07 tc07
 1:3 2:3
.o t08@b t08 tc08
 1:3 2:3
.o t09@b t09 tc09
 1:3 2:3
.o t10@b t10 tc10
 1:3 2:3
.o t11@b t11 tc11
 1:3 2:3
.o t12@b t12 tc12
 1:3 2:3
.o t13@b t13 tc13
 1:3 2:3
.o t14@b t14 tc14
 1:3 2:3
.o t15@b t15 tc15
 1:3 2:3
.o t16@b t16 tc16
 1:3 2:3
.o t17@b t17 tc17
 1:3 2:3
.o t18@b t18 tc18
 1:3 2:3
.o t19@b t19 tc19
 1:3 2:3
.o t20@b t20 tc20
 1:3 2:3
.o t21@b t21 tc21
 1:3 2:3
.o t22@b t22 tc22
 1:3 2:3
.o t23@b t23 tc23
 1:3 2:3
.o tc00@b
 0:0
.o tc01@b t00 tc00
 3:3
.o tc02@b t01 tc01
 3:3
.o tc03@b
 0:0
.o tc04@b t03 tc03
 3:3
.o tc05@b t04 tc04
 3:3
.o tc06@b t05 tc05
 3:3
.o tc07@b t06 tc06
 3:3
.o tc08@b t07 tc07
 3:3
.o tc09@b t08 tc08
 3:3
.o tc10@b t09 tc09
 3:3
.o tc11@b t10 tc10
 3:3
.o tc12@b t11 tc11
 3:3
.o tc13@b t12 tc12
 3:3
.o tc14@b t13 tc13
 3:3
.o tc15@b t14 tc14
 3:3
.o tc16@b t15 tc15
 3:3
.o tc17@b t16 tc16
 3:3
.o tc18@b t17 tc17
 3:3
.o tc19@b t18 tc18
 3:3
.o tc20@b t19 tc19
 3:3
.o tc21@b t20 tc20
 3:3
.o tc22@b t21 tc21
 3:3
.o tc23@b t22 tc22
 3:3
.o scaler@b t00 t01 t02
 7:7
.o hilo@b go hilo
 1:3 2:3
.o r0@b r0
 0:1
.o r1@b r0 r1
 1:3 2:3
.o s0@b s0
 0:1
.o s1@b s0 s1
 1:3 2:3
.o fire@b fire
 0:1
.o go@b fire frame
 3:3
.o shift0@b go hilo tx08 tx00 frame shift1
 32:49 7:7 9:11
.o shift1@b go hilo tx09 tx01 frame shift2
 32:49 7:7 9:11
.o shift2@b go hilo tx10 tx02 frame shift3
 32:49 7:7 9:11
.o shift3@b go hilo tx03 frame shift4
 16:25 5:7
.o shift4@b go hilo s1 tx04 frame shift5
 32:49 7:7 9:11
.o shift5@b go hilo s0 tx05 frame shift6
 32:49 7:7 9:11
.o shift6@b go hilo tx06 frame shift7
 16:25 5:7
.o shift7@b go hilo tx07 frame shift8
 16:25 5:7
.o shift8@b go hilo frame xin
 8:13 3:3
.o div0@b
 0:0
.o div1@b div1 div0 div2 div3
 2:10 6:6 3:3
.o div2@b div0 div1
 3:3
.o div3@b div3 div0 div1 div2
 3:15 14:14
.o frame@b div0 div1 div2 div3
 9:15
.o prev@b smin
 1:1
.o gate@b lock div0 div1 div2 div3
 18:30 0:1
.o indel@b smin
 1:1
.o samp@b lock smin indel
 4:5 3:3
.o xc0@b
 0:0
.o xc1@b x00 xc0
 3:3
.o xc2@b x01 xc1
 3:3
.o xc3@b x02 xc2
 3:3
.o xc4@b x03 xc3
 3:3
.o xc5@b x04 xc4
 3:3
.o xc6@b x05 xc5
 3:3
.o xc7@b x06 xc6
 3:3
.o xc8@b x07 xc7 bump big
 4:12 3:3
.o xc9@b x08 xc8
 3:3
.o incx0@b x00 xc0
 1:3 2:3
.o incx1@b x01 xc1
 1:3 2:3
.o incx2@b x02 xc2
 1:3 2:3
.o incx3@b x03 xc3
 1:3 2:3
.o incx4@b x04 xc4 bump big
 4:15 7:15 9:11 10:11 1:7 2:7
.o incx5@b x05 xc5 bump big
 4:15 7:15 9:11 10:11 1:7 2:7
.o incx6@b x06 xc6 bump big
 4:15 7:15 9:11 10:11 1:7 2:7
.o incx7@b x07 xc7 bump
 1:7 2:7 4:7 7:7
.o incx8@b x08 xc8 bump big
 12:15 15:15 1:11 2:11 1:7 2:7
.o incx9@b x09 xc9 bump big
 12:15 15:15 1:11 2:11 1:7 2:7
.o td-@i t04 t05 t06 t07 t08 t09 t10 t11 t12
 0:511
.o sout sout frame shift0
 1:7 4:5 2:3
.o sreq frame xin sreq
 3:3 4:4
.o dclk t03
 1:1
.o qclk t00
 1:1
.o x00 xin incx0 shift1
 4:5 3:3
.o x01 xin incx1 shift2
 4:5 3:3
.o x02 xin incx2 shift3
 4:5 3:3
.o x03 xin incx3 shift4
 4:5 3:3
.o x04 xin incx4 shift5
 4:5 3:3
.o x05 xin incx5 shift6
 4:5 3:3
.o x06 xin incx6 x00
 4:5 3:3
.o x07 xin incx7 x01
 4:5 3:3
.o x08 xin incx8 x02
 4:5 3:3
.o x09 xin incx9 x03
 4:5 3:3
.o x10 xin x04
 2:3
.o x11 xin x05
 2:3
.o y0 x06
 1:1
.o y1 x07
 1:1
.o y2 x08
 1:1
.o y3 x09
 1:1
.o y4 x10
 1:1
.o y5 x11
 1:1
.o y6 y0
 1:1
.o y7 y1
 1:1
.o comp div3 div2 div1 div0
 0:13 0:3
.o phi gate smin samp
 3:7 5:7
.o odat0 shift1
 1:1
.o odat1 shift2
 1:1
.o odat2 shift3
 1:1
.o odat3 shift4
 1:1
.o odat4 shift5
 1:1
.o odat5 shift6
 1:1
.o odat6 shift7
 1:1
.o odat7 shift8
 1:1
.o div0@t clk
 1:1
.o div1@t clk
 1:1
.o div2@t clk
 1:1
.o div3@t clk
 1:1
.o comp@d clk
 0:1
.o gate@d clk
 1:1
.o samp@d clk
 1:1
.o samp@g lock div0 div1 div2 div3
 18:30 0:1
.o indel@d clk
 1:1
.o t00@d clk
 1:1
.o t01@d clk
 1:1
.o t02@d clk
 1:1
.o t03@d clk
 1:1
.o t04@d clk
 1:1
.o t05@d clk
 1:1
.o t06@d clk
 1:1
.o t07@d clk
 1:1
.o t08@d clk
 1:1
.o t09@d clk
 1:1
.o t10@d clk
 1:1
.o t11@d clk
 1:1
.o t12@d clk
 1:1
.o t13@d clk
 1:1
.o t14@d clk
 1:1
.o t15@d clk
 1:1
.o t16@d clk
 1:1
.o t17@d clk
 1:1
.o t18@d clk
 1:1
.o t19@d clk
 1:1
.o t20@d clk
 1:1
.o t21@d clk
 1:1
.o t22@d clk
 1:1
.o t23@d clk
 1:1
.o t00@g
 0:0
.o t01@g
 0:0
.o t02@g
 0:0
.o t03@g scaler
 1:1
.o t04@g scaler
 1:1
.o t05@g scaler
 1:1
.o t06@g scaler
 1:1
.o t07@g scaler
 1:1
.o t08@g scaler
 1:1
.o t09@g scaler
 1:1
.o t10@g scaler
 1:1
.o t11@g scaler
 1:1
.o t12@g scaler
 1:1
.o t13@g scaler
 1:1
.o t14@g scaler
 1:1
.o t15@g scaler
 1:1
.o t16@g scaler
 1:1
.o t17@g scaler
 1:1
.o t18@g scaler
 1:1
.o t19@g scaler
 1:1
.o t20@g scaler
 1:1
.o t21@g scaler
 1:1
.o t22@g scaler
 1:1
.o t23@g scaler
 1:1
.o td-@d clk
 1:1
.o r0@c td-
 0:1
.o r1@c td-
 0:1
.o s0@c td-
 0:1
.o s1@c td-
 0:1
.o r0@d tq
 0:1
.o r1@d tq
 0:1
.o s0@d clk
 1:1
.o s1@d clk
 1:1
.o fire@d r1 s1
 0:3 3:3
.o fire@c td-
 0:1
.o s0@g go hilo
 3:3
.o s1@g go hilo
 3:3
.o tx00@l gxn
 0:1
.o tx01@l gxn
 0:1
.o tx02@l gxn
 0:1
.o tx03@l gxn
 0:1
.o tx04@l gxn
 0:1
.o tx05@l gxn
 0:1
.o tx06@l gxn
 0:1
.o tx07@l gxn
 0:1
.o tx08@l gxn
 0:1
.o tx09@l gxn
 0:1
.o tx10@l gxn
 0:1
.o tz00@l gzn
 0:1
.o tz01@l gzn
 0:1
.o tz02@l gzn
 0:1
.o tz03@l gzn
 0:1
.o tz04@l gzn
 0:1
.o tz05@l gzn
 0:1
.o tz06@l gzn
 0:1
.o tz07@l gzn
 0:1
.o tz08@l gzn
 0:1
.o tz09@l gzn
 0:1
.o tz10@l gzn
 0:1
.o ty00@l gyn
 0:1
.o ty01@l gyn
 0:1
.o ty02@l gyn
 0:1
.o ty03@l gyn
 0:1
.o ty04@l gyn
 0:1
.o ty05@l gyn
 0:1
.o ty06@l gyn
 0:1
.o ty07@l gyn
 0:1
.o ty08@l gyn
 0:1
.o ty09@l gyn
 0:1
.o ty10@l gyn
 0:1
.o prev@d clk
 1:1
.o hilo@d clk
 1:1
.o hilo@c td-
 0:1
.o shift0@d clk
 1:1
.o shift1@d clk
 1:1
.o shift2@d clk
 1:1
.o shift3@d clk
 1:1
.o shift4@d clk
 1:1
.o shift5@d clk
 1:1
.o shift6@d clk
 1:1
.o shift7@d clk
 1:1
.o shift8@d clk
 1:1
.o sout@d clk
 1:1
.o odat0@d clk
 1:1
.o odat1@d clk
 1:1
.o odat2@d clk
 1:1
.o odat3@d clk
 1:1
.o odat4@d clk
 1:1
.o odat5@d clk
 1:1
.o odat6@d clk
 1:1
.o odat7@d clk
 1:1
.o odat0@g frame
 1:1
.o odat1@g frame
 1:1
.o odat2@g frame
 1:1
.o odat3@g frame
 1:1
.o odat4@g frame
 1:1
.o odat5@g frame
 1:1
.o odat6@g frame
 1:1
.o odat7@g frame
 1:1
.o sreq@d clk
 1:1
.o sreq@c sack
 1:1
.o x00@g enx
 0:1
.o x01@g enx
 0:1
.o x02@g enx
 0:1
.o x03@g enx
 0:1
.o x04@g enx
 0:1
.o x05@g enx
 0:1
.o x06@g enx
 0:1
.o x07@g enx
 0:1
.o x08@g enx
 0:1
.o x09@g enx
 0:1
.o x10@g enx
 0:1
.o x11@g enx
 0:1
.o x00@d clk
 1:1
.o x01@d clk
 1:1
.o x02@d clk
 1:1
.o x03@d clk
 1:1
.o x04@d clk
 1:1
.o x05@d clk
 1:1
.o x06@d clk
 1:1
.o x07@d clk
 1:1
.o x08@d clk
 1:1
.o x09@d clk
 1:1
.o x10@d clk
 1:1
.o x11@d clk
 1:1
.o y0@g eny
 0:1
.o y1@g eny
 0:1
.o y2@g eny
 0:1
.o y3@g eny
 0:1
.o y4@g eny
 0:1
.o y5@g eny
 0:1
.o y6@g eny
 0:1
.o y7@g eny
 0:1
.o y0@d clk
 1:1
.o y1@d clk
 1:1
.o y2@d clk
 1:1
.o y3@d clk
 1:1
.o y4@d clk
 1:1
.o y5@d clk
 1:1
.o y6@d clk
 1:1
.o y7@d clk
 1:1
