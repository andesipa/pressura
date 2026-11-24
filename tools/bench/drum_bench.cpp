// Drum bus bench: compares Pressura settings with reference compressor models on real drum loops.
// Not part of the build. Loops: Ableton Core Library (Samples/Loops/Drums/Full), converted with
//   afconvert -f WAVE -d LEF32@48000 -c 2 "<loop>" <dir>/<Name>.wav   (names in loops[] below)
// Build and run (from OPTO2/):
//   clang++ -std=c++17 -O2 -I Source tools/bench/drum_bench.cpp -o /tmp/bench
//   REFS=1 /tmp/bench <dir> 25 80 1.5 100 0 0        # args: peak speed output mix limit vis (repeatable)
//   GRID=1 /tmp/bench <dir>                           # search settings closest to the FET 4:1 profile
//   env: LOOPS=a,b  TARGET=low,mid,high,bg,hits,kickPk  GRMIN/GRMAX  LIMIT=0|1  VIS=1  MIXES=30,40  PKSTEP=5
// Loops are normalised to -6 dBFS peak (the preset calibration level) and looped to 14 s.
// Drum bus bench: real loops (peak -6 dBFS) through Pressura (same math as processBlock) and reference
// models (FET 1176-style, VCA SSL-bus-style). Metrics on the mono sum after 2 s.
#include "OptoCompressor.h"
#include "Saturator.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
const double sr=48000;
using Buf=std::vector<float>;
struct St{Buf l,r;};
St readWav(const char*p){ FILE*f=fopen(p,"rb"); std::vector<char> d; char b[65536]; size_t n; while((n=fread(b,1,sizeof b,f))>0) d.insert(d.end(),b,b+n); fclose(f);
 size_t i=12; while(i+8<d.size()){ uint32_t sz; memcpy(&sz,&d[i+4],4); if(!memcmp(&d[i],"data",4)){ size_t cnt=sz/8; St s; s.l.resize(cnt); s.r.resize(cnt); const float*x=(const float*)&d[i+8]; for(size_t k=0;k<cnt;++k){s.l[k]=x[2*k]; s.r[k]=x[2*k+1];} return s;} i+=8+sz+(sz&1);} return {}; }
St prep(St s){ float pk=0; for(size_t i=0;i<s.l.size();++i) pk=std::max({pk,std::abs(s.l[i]),std::abs(s.r[i])}); float g=0.5012f/pk; St o; size_t L=s.l.size(); size_t N=(size_t)(sr*14);
 o.l.resize(N); o.r.resize(N); for(size_t i=0;i<N;++i){o.l[i]=g*s.l[i%L]; o.r[i]=g*s.r[i%L];} return o; }
// ---- metrics ----
struct Bq{double b0,b1,b2,a1,a2,z1=0,z2=0; double p(double x){double y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y;}};
Bq lp(double f){double w=2*M_PI*f/sr,c=cos(w),s=sin(w),al=s/(2*0.7071),a0=1+al; return {(1-c)/2/a0,(1-c)/a0,(1-c)/2/a0,-2*c/a0,(1-al)/a0};}
Bq hp(double f){double w=2*M_PI*f/sr,c=cos(w),s=sin(w),al=s/(2*0.7071),a0=1+al; return {(1+c)/2/a0,-(1+c)/a0,(1+c)/2/a0,-2*c/a0,(1-al)/a0};}
Bq shelf(double f,double g){double A=pow(10,g/40),w=2*M_PI*f/sr,c=cos(w),s=sin(w),al=s/2*sqrt(2),sA=2*sqrt(A)*al; double a0=(A+1)-(A-1)*c+sA;
 return {A*((A+1)+(A-1)*c+sA)/a0,-2*A*((A-1)+(A+1)*c)/a0,A*((A+1)+(A-1)*c-sA)/a0,2*((A-1)-(A+1)*c)/a0,((A+1)-(A-1)*c-sA)/a0};}
struct M{double loud,low,mid,high,bg,hits,lowPk,peak;};
M measure(const St&s){ size_t N=s.l.size(), st=(size_t)(2*sr); Bq k1=shelf(1500,4),k2=hp(38),l1=lp(120),l2=lp(120),m1=hp(120),m2=hp(120),m3=lp(2500),m4=lp(2500),h1=hp(2500),h2=hp(2500);
 double sk=0,sl=0,sm=0,sh=0,pk=0; std::vector<double> win,lowWin; double acc=0,lacc=0; int wn=(int)(0.02*sr),c=0;
 for(size_t i=0;i<N;++i){ double x=0.5*(s.l[i]+s.r[i]); double k=k2.p(k1.p(x)); double lo=l2.p(l1.p(x)); double mi=m4.p(m3.p(m2.p(m1.p(x)))); double hi=h2.p(h1.p(x));
  if(i<st) continue; sk+=k*k; sl+=lo*lo; sm+=mi*mi; sh+=hi*hi; pk=std::max(pk,std::abs(x)); acc+=x*x; lacc+=lo*lo; if(++c==wn){win.push_back(10*log10(acc/wn+1e-12)); lowWin.push_back(10*log10(lacc/wn+1e-12)); acc=lacc=0;c=0;} }
 double n=N-st; auto pct=[](std::vector<double> v,double p){std::sort(v.begin(),v.end()); return v[(size_t)(p*(v.size()-1))];};
 return {10*log10(sk/n),10*log10(sl/n),10*log10(sm/n),10*log10(sh/n),pct(win,0.40),pct(win,0.95),pct(lowWin,0.95),20*log10(pk)}; }
// ---- processors: return gain-in-dB per sample from detector; generic apply with mix ----
struct Res{St out; double grP95;};
Res applyGain(const St&in,const std::vector<float>&gdb,float mix,float outDb,bool sat=false,float satAmt=0){ St o=in; Saturator S; S.setAmount(satAmt); S.prepare(sr); float og=pow(10,outDb/20); std::vector<double> gr;
 for(size_t i=0;i<in.l.size();++i){ float g=pow(10,gdb[i]/20); float wl=in.l[i]*g,wr=in.r[i]*g; if(sat) S.process(wl,wr); o.l[i]=og*(in.l[i]+mix*(wl-in.l[i])); o.r[i]=og*(in.r[i]+mix*(wr-in.r[i])); } return {o,0}; }
// Pressura
struct PP{float peak,speed,out,mix; bool lim,vis;};
Res pressura(const St&in,PP p){ OptoCompressor c; c.setPeakReduction(p.peak); c.setSpeed(p.speed); c.setLimitMode(p.lim); c.setDriveMode(p.vis); c.prepare(sr);
 std::vector<float> g(in.l.size()); std::vector<double> gr; for(size_t i=0;i<g.size();++i){ g[i]=c.processDb(in.l[i],in.r[i]); if(i>sr*2) gr.push_back(c.getGainReductionDb()); }
 auto r=applyGain(in,g,p.mix/100,p.out,p.vis,p.peak); std::sort(gr.begin(),gr.end()); r.grP95=gr[(size_t)(0.95*(gr.size()-1))]; return r; }
// Generic feed-forward compressor in the log domain (used for the FET and VCA references)
struct Ref{const char*name; double thr,ratio,knee,attMs,relMs,relSlowMs; bool peakDet; double rmsMs; float mix;};
std::vector<float> refGain(const St&in,const Ref&R,double thr,std::vector<double>*grOut){ double aA=exp(-1/(R.attMs*1e-3*sr)),aR=exp(-1/(R.relMs*1e-3*sr)),aS=R.relSlowMs>0?exp(-1/(R.relSlowMs*1e-3*sr)):0,aRms=exp(-1/(R.rmsMs*1e-3*sr));
 double env=0,gr=0,grS=0; std::vector<float> g(in.l.size());
 for(size_t i=0;i<g.size();++i){ double x=std::max(std::abs(in.l[i]),std::abs(in.r[i])); double lvl;
  if(R.peakDet){ lvl=20*log10(x+1e-9);} else { env=x*x+aRms*(env-x*x); lvl=10*log10(env+1e-12);}
  double over=lvl-thr,sl=1-1/R.ratio,t; if(2*over<=-R.knee) t=0; else if(2*over>=R.knee) t=sl*over; else {double k=over+R.knee/2; t=sl*k*k/(2*R.knee);}
  if(t>gr) gr=t+aA*(gr-t); else gr=t+aR*(gr-t);
  double out=gr; if(R.relSlowMs>0){ if(t>grS) grS=t+aA*(grS-t); else grS=t+aS*(grS-t); out=std::max(gr,0.5*grS);} // SSL-auto-ish two stage
  g[i]=-out; if(grOut&&i>sr*2) grOut->push_back(out);} return g; }
Res reference(const St&in,const Ref&R,double targetGr){ double lo=-60,hi=0,thr=-20; std::vector<double> gr;
 for(int it=0;it<30;++it){ thr=0.5*(lo+hi); gr.clear(); refGain(in,R,thr,&gr); std::sort(gr.begin(),gr.end()); double p=gr[(size_t)(0.95*(gr.size()-1))]; if(p>targetGr) lo=thr; else hi=thr; }
 auto g=refGain(in,R,thr,&gr); std::sort(gr.begin(),gr.end()); auto r=applyGain(in,g,R.mix,0); r.grP95=gr[(size_t)(0.95*(gr.size()-1))]; return r; }
St gainSt(St s,double db){ float g=pow(10,db/20); for(auto&x:s.l)x*=g; for(auto&x:s.r)x*=g; return s; }
std::vector<std::string> loops={"Hip_Hop_Shuffle_Dry_90","Break_Phenom_84","80s_Beat_90","Hip_Hop_Congas_100","Break_Ghosts_90","House_Beat_122"};
int main(int argc,char**argv){ std::string dir=argv[1];
 if(const char*e=getenv("LOOPS")){ loops.clear(); std::string l=e; size_t a=0,b; while((b=l.find(',',a))!=std::string::npos){loops.push_back(l.substr(a,b-a)); a=b+1;} loops.push_back(l.substr(a)); } std::vector<St> ins; std::vector<M> inM; for(auto n:loops){ ins.push_back(prep(readWav((dir+"/"+n+".wav").c_str()))); inM.push_back(measure(ins.back())); }
 bool perLoop=getenv("PERLOOP")!=nullptr;
 auto report=[&](const char*name,std::function<Res(const St&)> f){ double a[9]={0}; for(size_t k=0;k<ins.size();++k){ Res r=f(ins[k]); M m=measure(r.out); M&i=inM[k]; double L=m.loud-i.loud;
   double v[9]={L,m.low-i.low-L,m.mid-i.mid-L,m.high-i.high-L,m.bg-i.bg-L,m.hits-i.hits-L,m.lowPk-i.lowPk-L,m.peak,r.grP95};
   if(perLoop) printf("   %-24s %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %6.1f %5.1f\n",loops[k].c_str(),v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8]);
   for(int j=0;j<9;++j) a[j]+=v[j]/ins.size(); }
  printf("%-34s %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %6.1f %5.1f\n",name,a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]); };
 printf("Balance = change vs input after matching loudness (dB). loud = raw loudness change. bg = quiet parts (P40 20 ms), hits = P95, kickPk = low band P95.\n");
 printf("%-34s %6s %6s %6s %6s %6s %6s %6s %6s %5s\n","processor","loud","low","mid","high","bg","hits","kickPk","peak","GR95");
 for(int a=2;a+5<argc;a+=6){ PP p{(float)atof(argv[a]),(float)atof(argv[a+1]),(float)atof(argv[a+2]),(float)atof(argv[a+3]),(bool)atoi(argv[a+4]),(bool)atoi(argv[a+5])}; char nm[64]; snprintf(nm,64,"Pressura %g/%g/%+g/%g%s%s",p.peak,p.speed,p.out,p.mix,p.lim?" L":"",p.vis?" V":""); report(nm,[p](const St&s){return pressura(s,p);}); }
 if(getenv("GRID")){ // distance to the FET 4:1 800us/100ms profile
  double tgt[6]={-0.3,0.2,1.2,2.9,-0.8,-0.9}; double grMin=3,grMax=6.5;
  if(const char*t=getenv("TARGET")) sscanf(t,"%lf,%lf,%lf,%lf,%lf,%lf",&tgt[0],&tgt[1],&tgt[2],&tgt[3],&tgt[4],&tgt[5]);
  if(getenv("GRMIN")) grMin=atof(getenv("GRMIN")); if(getenv("GRMAX")) grMax=atof(getenv("GRMAX"));
  std::vector<int> lims={0,1}; if(getenv("LIMIT")) lims={atoi(getenv("LIMIT"))}; bool vis=getenv("VIS")&&atoi(getenv("VIS"));
  std::vector<float> mixes={60,80,100}; if(getenv("MIXES")){ mixes.clear(); std::string m=getenv("MIXES"); size_t a=0,b; while((b=m.find(',',a))!=std::string::npos){mixes.push_back(atof(m.substr(a,b-a).c_str())); a=b+1;} mixes.push_back(atof(m.substr(a).c_str())); } struct Row{double d; PP p; double v[9];}; std::vector<Row> rows;
  for(float pk=0;pk<=100;pk+=getenv("PKSTEP")?atof(getenv("PKSTEP")):5) for(float sp:{20.f,30.f,40.f,50.f,60.f,70.f,80.f,90.f,100.f}) for(float mx:mixes) for(int lim:lims){
   PP p{pk,sp,0,mx,(bool)lim,vis}; double a[9]={0};
   for(size_t k=0;k<ins.size();++k){ Res r=pressura(ins[k],p); M m=measure(r.out); M&i=inM[k]; double L=m.loud-i.loud;
    double v[9]={L,m.low-i.low-L,m.mid-i.mid-L,m.high-i.high-L,m.bg-i.bg-L,m.hits-i.hits-L,m.lowPk-i.lowPk-L,m.peak,r.grP95}; for(int j=0;j<9;++j) a[j]+=v[j]/ins.size(); }
   double d=0; for(int j=0;j<6;++j) d+=(a[j+1]-tgt[j])*(a[j+1]-tgt[j]); if(a[8]>grMax||a[8]<grMin) d+=10; Row R{d,p}; memcpy(R.v,a,sizeof a); rows.push_back(R); }
  std::sort(rows.begin(),rows.end(),[](auto&a,auto&b){return a.d<b.d;});
  for(int i=0;i<12;++i){auto&r=rows[i]; printf("d=%.2f %3.0f/%3.0f/mix%3.0f%s  %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %+6.1f %6.1f %5.1f\n",r.d,r.p.peak,r.p.speed,r.p.mix,r.p.lim?" L":"  ",r.v[0],r.v[1],r.v[2],r.v[3],r.v[4],r.v[5],r.v[6],r.v[7],r.v[8]);}
 }
 if(getenv("REFS")){
  std::vector<std::pair<Ref,double>> refs={
   {{"FET 4:1 att 800us rel 100ms",0,4,6,0.8,100,0,true,0,1},5},
   {{"FET 4:1 att 400us rel 250ms",0,4,6,0.4,250,0,true,0,1},5},
   {{"FET all-btn 20:1 par 40%",0,20,2,0.6,80,0,true,0,0.4f},10},
   {{"VCA 4:1 att 30ms rel auto",0,4,4,30,100,1200,false,3,1},4},
   {{"VCA 4:1 att 10ms rel 100ms",0,4,4,10,100,0,false,3,1},4},
   {{"VCA 2:1 att 30ms rel auto",0,2,4,30,100,1200,false,3,1},3},
   {{"VCA 2:1 att 30ms auto 1.5dB",0,2,4,30,100,1200,false,3,1},1.5},
   {{"VCA 4:1 att 30ms auto 2dB",0,4,4,30,100,1200,false,3,1},2},
   {{"FET all-btn 20:1 100% 10dB",0,20,2,0.6,80,0,true,0,1},10}};
  for(auto&pr:refs){ Ref R=pr.first; double t=pr.second; report(R.name,[R,t](const St&s){ return reference(s,R,t); }); }
 } }
