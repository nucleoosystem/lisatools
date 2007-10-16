/*
Copyright (C) 2006  S. Babak

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/          
/******************  CVS info ************************ 
#define CVSTAG "$Name:  $"
#define CVSHEADER "$Header: /afs/aeiw/cvsroot/waves/people/stas/LISAWP/waveforms/src/SpinBBHWaveform.cc,v 1.1 2007/09/18 21:56:08 stba Exp $" 
*/

#include "SpinBBHWaveform.hh"

namespace LISAWP{

SpinBBHWaveform::SpinBBHWaveform(double mass1, double mass2, double chi1, double chi2, double timeStep){

  m1 = mass1;
  m2 = mass2;
  x1 = chi1;
  x2 = chi2;
  dt = timeStep;
  M = m1 + m2;
  m1M2 = m1*m1/(M*M);
  m2M2 = m2*m2/(M*M);
  eta = m1*m2/(M*M);
  m1 *= LISAWP_MTSUN_SI; // mass in seconds
  m2 *= LISAWP_MTSUN_SI; 
  M *= LISAWP_MTSUN_SI;
  mu = m1*m2/M;
  dm = m1-m2;
  
  observerSet = false;
  nonspin = false; 
  runDone = false; 
  MECO = true;
  back = false;
  if(x1 == 0.0 && x2 == 0.0)
 	   nonspin = true;

}


void SpinBBHWaveform::ComputeInspiral(double t0,  double Tc, double phiC, double iota0, double alpha0, \
                                       double S1z0, double phiS10,  double S2z0, double phiS20, \
                                       double maxDuration){

  LISAWPAssert( S1z >= -1.0 && S1z <=1.0, "S1z is outside [-1,1]" );
  LISAWPAssert( S2z >= -1.0 && S2z <=1.0, "S2z is outside [-1,1]" );

    
  iota = iota0;
  alpha = alpha0;
  PhiOrbC = 0.5*phiC;

  Lnx = sin(iota)*cos(alpha);
  Lny = sin(iota)*sin(alpha);
  Lnz = cos(iota);

  if(iota == 0.0){
     nonspin = true;
     x1 = 0.0;
     x2 = 0.0;
  }
  if (nonspin == true){
        iota= 0.0;
        alpha = 0.0;
  }
  
  S1z = S1z0;
  double thetaS1 = acos(S1z0);
  S1x = sin(thetaS1)*cos(phiS10);
  S1y = sin(thetaS1)*sin(phiS10);
  S2z = S2z0;
  double thetaS2 = acos(S2z0);
  S2x = sin(thetaS2)*cos(phiS20);
  S2y = sin(thetaS2)*sin(phiS20);

  om = ComputeOrbFreq(0.0, Tc);
  om_prev = om;
  Phi = ComputeOrbPhase(om, PhiOrbC);
  
  // Computing direction of the total angular momentum
  
  double Jx = eta*M*M*pow(M*om, -1./3)*Lnx + S1x + S2x;
  double Jy = eta*M*M*pow(M*om, -1./3)*Lny + S1y + S2y;
  double Jz = eta*M*M*pow(M*om, -1./3)*Lnz + S1z + S2z;
  double absJ = sqrt(Jx*Jx + Jy*Jy + Jz*Jz);
  
  LISAWPAssert(absJ > 0.0, "total angular momentum iz zero at t=0");
  thetaJ = acos(Jz/absJ);
  if(fabs(thetaJ) <= 1.e-6 || fabs(thetaJ - LISAWP_PI) <= 1.e-6){
      phiJ = 0.0;
  }else{
      phiJ = atan2(Jy, Jx);
  }
  
  

  om_lso = pow(6,-1.5)/M;
  
  double  t = 0.0;
  double advstep, cumulstep, newstep;

  Matrix<double> coord0(1, 12);
  Matrix<double> coordn(1, 12);
  Matrix<double> rhs(1,12);
  rhs = 0.0;
  coordn = 0.0;

  coord0(0) = iota;
  coord0(1) = alpha;
  coord0(2) = Lnx;
  coord0(3) = Lny;
  coord0(4) = Lnz;
  coord0(5) = S1x;
  coord0(6) = S1y;
  coord0(7) = S1z;
  coord0(8) = S2x;
  coord0(9) = S2y;
  coord0(10) = S2z;
  coord0(11) = 0.0;
  
  int counter = 1;
  newstep = dt;
  cumulstep = dt;  
  
  if (om >= om_lso)
           std::cout << "inital frequncy = " << om << "  lso frequency = " << om_lso << std::endl;
  LISAWPAssert(om < om_lso, "LSO reached with initial conditions");
  MECO = CheckMECO();
  if(MECO){
        std::cerr << "MECO/LSO reached with initial conditioins" << std::endl;
        exit(1);
  }

  std::vector<double> tn;
  std::vector<double> Phase_n;
  std::vector<double> freq_n;
  std::vector<double> io_n;
  std::vector<double> al_n;
  std::vector<double> s1x_n;
  std::vector<double> s1y_n;
  std::vector<double> s1z_n;
  std::vector<double> s2x_n;
  std::vector<double> s2y_n;
  std::vector<double> s2z_n;
  
  Derivs(t, coord0, rhs, 12);
  double prec0 = -rhs(1)*cos(iota);
  coord0(11) = prec0;
     
     // ---------------------------------
     //  ***   backward integration  ***
     // ---------------------------------
  if (t0 < 0.0){
         back = true;
         while (t >= t0){
             Derivs(t, coord0, rhs, 12);
             advstep =  IntegratorCKRK(coord0, rhs, coordn, t, dt, 12); 
             iota = coordn(0);
             alpha = coordn(1);
             Lnx = coordn(2);
             Lny = coordn(3);
             Lnz = coordn(4);
             S1x = coordn(5);
             S1y = coordn(6);
             S1z = coordn(7);
             S2x = coordn(8);
             S2y = coordn(9);
             S2z = coordn(10);
             // computing phase and freq.
             t = t - dt;
             om = ComputeOrbFreq(t, Tc);
             Phi = ComputeOrbPhase(om, PhiOrbC) + coordn(11);
             coord0 = coordn;
            
             if (t >= t0){
                 tn.push_back(t);
                 Phase_n.push_back(Phi);
                 freq_n.push_back(om);
                 io_n.push_back(iota);
                 al_n.push_back(alpha);
                 s1x_n.push_back(S1x);
                 s1y_n.push_back(S1y);
                 s1z_n.push_back(S1z);
                 s2x_n.push_back(S2x);
                 s2y_n.push_back(S2y);
                 s2z_n.push_back(S2z);
             }   
         }    
         for(int i = 0; i < (int)tn.size(); i++){
              int k = tn.size() - i -1;
              time.push_back(tn[k]);
              Phase.push_back(Phase_n[k]);  
              freq.push_back(freq_n[k]);
              io.push_back(io_n[k]);
              al.push_back(al_n[k]);
              s1x.push_back(s1x_n[k]);
              s1y.push_back(s1y_n[k]);
              s1z.push_back(s1z_n[k]);
              s2x.push_back(s2x_n[k]);
              s2y.push_back(s2y_n[k]);
              s2z.push_back(s2z_n[k]);
         }
     
         tn.resize(0);
         Phase_n.resize(0);
         freq_n.resize(0);
         io_n.resize(0);
         al_n.resize(0);
         s1x_n.resize(0);
         s1y_n.resize(0);
         s1z_n.resize(0);
         s2x_n.resize(0);
         s2y_n.resize(0);
         s2z_n.resize(0);
     
         t = 0.0;
         iota = iota0;
         alpha = alpha0;

         Lnx = sin(iota)*cos(alpha);
         Lny = sin(iota)*sin(alpha);
         Lnz = cos(iota);
         S1z = S1z0;
         S1x = sin(thetaS1)*cos(phiS10);
         S1y = sin(thetaS1)*sin(phiS10);
         S2z = S2z0;
         S2x = sin(thetaS2)*cos(phiS20);
         S2y = sin(thetaS2)*sin(phiS20);
         om = ComputeOrbFreq(t, Tc);
         
         coord0(0) = iota;
         coord0(1) = alpha;
         coord0(2) = Lnx;
         coord0(3) = Lny;
         coord0(4) = Lnz;
         coord0(5) = S1x;
         coord0(6) = S1y;
         coord0(7) = S1z;
         coord0(8) = S2x;
         coord0(9) = S2y;
         coord0(10) = S2z;
         coord0(11) = prec0;
  }// end of the backward integration
    // ----- Forward integration  ----
  back =  false; 
    
  while (t <= maxDuration){
     
       time.push_back(t);
       freq.push_back(om);
       Phi = ComputeOrbPhase(om, PhiOrbC) + coord0(11);
       Phase.push_back(Phi);
       io.push_back(iota);
       al.push_back(alpha);
       s1x.push_back(S1x);
       s1y.push_back(S1y);
       s1z.push_back(S1z);
       s2x.push_back(S2x);
       s2y.push_back(S2y);
       s2z.push_back(S2z);
    
      /* fout33 << t << spr << om << spr << Phi << spr << iota << spr << alpha
   	      << spr << S1x << spr << S1y << spr << S1z << spr << S2x << spr << 
	      S2y << spr << S2z << spr << coord0(3) << spr << coord0(4) << spr <<
	      coord0(5) << spr << sin(iota)*cos(alpha) << spr << sin(iota)*sin(alpha)
	     << spr << cos(iota) <<  std::endl;
    */
       Derivs(t, coord0, rhs, 12);
       advstep =  IntegratorCKRK(coord0, rhs, coordn, t, dt, 12);
       counter ++;
       cumulstep += advstep;
       if (advstep < newstep)
           newstep = advstep;

       iota = coordn(0);
       alpha = coordn(1);
       Lnx = coordn(2);
       Lny = coordn(3);
       Lnz = coordn(4);
       S1x = coordn(5);
       S1y = coordn(6);
       S1z = coordn(7);
       S2x = coordn(8);
       S2y = coordn(9);
       S2z = coordn(10);
       t += dt;
       om =  ComputeOrbFreq(t, Tc);

       MECO = CheckMECO();
       if (MECO){
          std::cout << "*** we reached merger condition at t = " << t << std::endl;
          Tc = t;
          break;
       }
       coord0 = coordn;  //take the final coordinates as initial for the next step
      /* std::cout  << t  << "     "
	  << om << "      "  << dEdomega*1e5 << std::endl; 
      */	    
  }//while loop	     
  runDone = true;

}

bool SpinBBHWaveform::CheckMECO(){

   // requires current values of freq, spins, Ln.
	
   bool nonstab;
   nonstab = false;

// We desable MECO condition and use d\omega/dt>0 and LSO conditions

  /* double Mom = M*om;
   double S1S2L = (S1y*S2z - S1z*S2y)*Lnx + (S1z*S2x - S1x*S2z)*Lny + (S1x*S2y - S1y*S2x)*Lnz;
   double S1S2 = S1x*S2x + S1y*S2y + S1z*S2z;
   double LS1 = Lnx*S1x + Lny*S1y + Lnz*S1z;
   double LS2 = Lnx*S2x + Lny*S2y + Lnz*S2z;
   double LnSeff = Lnx*( x1*m1M2*(1.0 + 0.75*(m2/m1) )*S1x + x2*m2M2*(1.0 + 0.75*(m1/m2))*S2x) + \
		   Lny*( x1*m1M2*(1.0 + 0.75*(m2/m1) )*S1y + x2*m2M2*(1.0 + 0.75*(m1/m2))*S2y) + \
		   Lnz*( x1*m1M2*(1.0 + 0.75*(m2/m1) )*S1z + x2*m2M2*(1.0 + 0.75*(m1/m2))*S2z);
    
   double Mom13 = pow(Mom, 1./3.);

   dEdomega = -(mu/3.0) * (1.0/Mom13)*( 1.0 - ((9.0+eta)/6.0)*Mom13*Mom13 + (20./3.)*LnSeff*Mom + \
	   (5./64.)*(1.-0.375*eta)*Mom13*(dm/M)*x1*x2*(1.0 + (743.0+924.0*eta)/336.0*Mom13*Mom13)*S1S2L +\
		 0.125*(-81. + 57.*eta - eta*eta)*Mom*Mom13 + \
		 0.75*eta*x1*x2*( S1S2 - 3.*LS1*LS2 )*Mom*Mom13 + \
		 4.0*( -675./64. + (34445./576. - (205./96.)*LISAWP_PI*LISAWP_PI)*eta - (155./96.)*eta*eta -\
		    (35./5184.)*eta*eta*eta )*Mom*Mom  );
   
   if (fabs(dEdomega)*1.e5 <= 3.5e-1  || dEdomega >= 0.0 || om < om_prev){
	   std::cout << "MECO reached at omega = " << om << std::endl;
	   std::cout << " dEdomega =  "  << dEdomega << std::endl;
	   
	   nonstab = true;
   }
   */
   if(om >= om_lso){
       nonstab = true;
       std::cout << "LSO reached at omega = " << om << std::endl;
   }
   
   if(om < om_prev){
       std::cout << "Instability condition is met at domega/dt = 0: orb. ang. freq. = " << om << std::endl;
       nonstab =  true;
   }
   
   om_prev = om;
   //std::cout << "test: om = " << om << "  dEdom =  " << dEdomega << std::endl;
   return(nonstab);
}


void SpinBBHWaveform::Derivs(double x, Matrix<double> y, Matrix<double>&dydx, int n){

    double Mom, Mom13;	
    double SpinOrb, SpinSpin, PN0, PN2, PN3, PN4;
    double VecS1x, VecS1y, VecS1z, VecS2x, VecS2y, VecS2z, VecLx, VecLy, VecLz;
	
    iota = y(0);
    alpha = y(1);
    Lnx = y(2);
    Lny = y(3);
    Lnz = y(4);
    S1x = y(5);
    S1y = y(6);
    S1z = y(7);
    S2x = y(8);
    S2y = y(9);
    S2z = y(10);

    if (fabs(Lnx - sin(iota)*cos(alpha)) >= 1.e-3 || fabs(Lny - sin(iota)*sin(alpha)) >= 1.e-3 || \
        fabs(Lnz - cos(iota)) >= 1.e-3){
	    std::cout << "Insufficient precisioin: " << Lnx - sin(iota)*cos(alpha) << "    " \
		    << Lny - sin(iota)*sin(alpha) << "     " << Lnz - cos(iota) << std::endl;
	    exit(1);
    }
    
    om = ComputeOrbFreq(x, Tc);
    
    Mom = om*M;
    Mom13 = pow(Mom, 1./3.);
    
  // modulation of S1
    VecS1x = Mom*Mom/(2.0*M) * ( (eta/Mom13)*(4.0 + 3.0*m2/m1)*Lnx + x2*m2M2*(S2x - 3.0*LS2*Lnx) );
  
    VecS1y = Mom*Mom/(2.0*M) * ( (eta/Mom13)*(4.0 + 3.0*m2/m1)*Lny + x2*m2M2*(S2y - 3.0*LS2*Lny) );
  
    VecS1z = Mom*Mom/(2.0*M) * ( (eta/Mom13)*(4.0 + 3.0*m2/m1)*Lnz + x2*m2M2*(S2z - 3.0*LS2*Lnz) );

  // modulation of S2
  
    VecS2x = Mom*Mom/(2.0*M) * ( (eta/Mom13)*(4.0 + 3.0*m1/m2)*Lnx + x1*m1M2*(S1x - 3.0*LS1*Lnx) );
  
    VecS2y = Mom*Mom/(2.0*M) * ( (eta/Mom13)*(4.0 + 3.0*m1/m2)*Lny + x1*m1M2*(S1y - 3.0*LS1*Lny) );
  
    VecS2z = Mom*Mom/(2.0*M) * ( (eta/Mom13)*(4.0 + 3.0*m1/m2)*Lnz + x1*m1M2*(S1z - 3.0*LS1*Lnz) );
 
  // modulation of Ln + secular decay
   
    VecLx = Mom*Mom/(2.0*M)* ( (4.0 + 3.0*m2/m1)*x1*m1M2*S1x + (4.0 + 3.0*m1/m2)*x2*m2M2*S2x  -\
                  3.0*Mom13*eta*x1*x2*( LS2*S1x + LS1*S2x) ) - 32./5.*eta*eta*M*Mom*Mom*Mom13*Lnx;
	  
    VecLy = Mom*Mom/(2.0*M)* ( (4.0 + 3.0*m2/m1)*x1*m1M2*S1y + (4.0 + 3.0*m1/m2)*x2*m2M2*S2y  -\
                  3.0*Mom13*eta*x1*x2*(LS2*S1y + LS1*S2y) ) - 32./5.*eta*eta*M*Mom*Mom*Mom13*Lny;
	  
    VecLz = Mom*Mom/(2.0*M)* ( (4.0 + 3.0*m2/m1)*x1*m1M2*S1z + (4.0 + 3.0*m1/m2)*x2*m2M2*S2z  -\
                  3.0*Mom13*eta*x1*x2*(LS2*S1z + LS1*S2z) ) - 32./5.*eta*eta*M*Mom*Mom*Mom13*Lnz;

    dydx(0) = VecLy*cos(alpha) - VecLx*sin(alpha);
     
    dydx(1) = VecLz - (cos(iota)/sin(iota))*( VecLx*cos(alpha) + VecLy*sin(alpha) );

    dydx(2) = VecLy*Lnz - VecLz*Lny;
    dydx(3) = VecLz*Lnx - VecLx*Lnz;
    dydx(4) = VecLx*Lny - VecLy*Lnx;
     
    dydx(5) = S1z*VecS1y - S1y*VecS1z;
    dydx(6) = S1x*VecS1z - S1z*VecS1x;
    dydx(7) = S1y*VecS1x - S1x*VecS1y;
	     
    dydx(8) = S2z*VecS2y - S2y*VecS2z;
    dydx(9) = S2x*VecS2z - S2z*VecS2x;
    dydx(10) = S2y*VecS2x - S2x*VecS2y;

    dydx(11) = - dydx(1)*cos(iota); // phase modulation 
         
 
    if(back){
      dydx *= (-1.);
    }

  
}

double SpinBBHWaveform::ComputeOrbFreq(double t, double tc){
    
    LS1 = Lnx*S1x + Lny*S1y + Lnz*S1z;
    LS2 = Lnx*S2x + Lny*S2y + Lnz*S2z;
    S1S2 = S1x*S2x + S1y*S2y + S1z*S2z;

    beta = (1.0/12.0)*( x1*LS1*(113.0*m1M2 + 75.0*eta) + \
    	                  x2*LS2*(113.0*m2M2 + 75.0*eta) );
    sigma = (1./48.0)*eta*x1*x2*( -247.0*S1S2 + 721.0*LS1*LS2 );
    
    double tau = 0.2*eta*(tc-t)/M;
    double tau38 = pow(tau, -0.375);
    double tau14 = pow(tau, -0.25);
    
    double x = 0.125*tau38*(1.0 + (743./2688. + 11.*eta/32.)*tau14 - \
            0.3*(LISAWP_PI - 0.25*beta)*tau38 + (1855099./14450688. + \
            56975./258048.*eta + 371./2048.*eta*eta - 3./64.*sigma)*tau14*tau14);
    
    return(x/M);        
    
}

double SpinBBHWaveform::ComputeOrbPhase(double om, double phi_C){
    
    double Mom = M*om;
    double v = pow(Mom, 2./3.);
    double phi_orb = phi_C - (1./32.)/(eta*v*Mom)*(1.0 + (3715./1008. + 55./12.*eta)*v \
                    - (10.*LISAWP_PI - 2.5*beta)*Mom  +\
    		    (15293365./1016064. + 27145./1008.*eta + 3085./144.*eta*eta - 5.*sigma)*v*v);
    return(phi_orb);    
}


void SpinBBHWaveform::SetObserver(double theta_s, double phi_s, double D){

     double ctS = cos(theta_s);
     double stS = sin(theta_s);
     double ctJ = cos(thetaJ);
     double stJ = sin(thetaJ);
     
     dist = D*LISAWP_PC_SI/LISAWP_C_SI; //distance in seconds
     
     theta = LISAWP_PI - acos(ctJ*ctS + stJ*stS*cos(phi_s - phiJ));
     if(fabs(thetaJ) <= 1.e-6 || fabs(thetaJ - LISAWP_PI) <= 1.e-6){
         psi = 0.5*LISAWP_PI;
     }else{
         double up = ctS*stJ*cos(phi_s-phiJ) - ctJ*stS;
         double down = stJ*sin(phi_s - phiJ);
         psi = atan2(up, down);
     }
   //  std::cout << "Stas: mu/D " << mu/dist << "   " << mu << "     " << dist << std::endl;
     
     observerSet = true;

}
	

int SpinBBHWaveform::ComputeWaveform(int order, double taper, \
	       	double* hPlus, long hPlusLength, double* hCross, long hCrossLength){

    double Qp, P05p, P1p, Qc, P05c, P1c; 
    double C, S, K, a, b, c, d, DC, DS, Dx, Dy, Dz;

    double cs, cs2, sn, sn2;
    double sa, ca, si, ci, si2, sa2;
    double Mom13, Mom23;
    
    LISAWPAssert(observerSet, "You must specify direction to observer");
    LISAWPAssert(runDone, "You must compute inspiraling trajectory first");

    int size = time.size();
    
    if (!nonspin){
       LISAWPAssert(Phase.size() == size && io.size() == size && al.size() == size, "Sizes do not match");
    }else{
       LISAWPAssert(Phase.size() == size, "Sizes do not match");
    }
    LISAWPAssert(size <= hPlusLength && size <= hCrossLength, "Buffer insufficient!");
  
    double hp, hc;

    cs = cos(theta);
    cs2 = cos(2.0*theta);
    sn = sin(theta);
    sn2 = sin(2.0*theta);
    
    double wk = 1.0;
    double xmax;
    if (taper != 0.0) xmax = 1./taper;  // hardcoded taper start at 7M
    double Ataper = 150;   // this parameter regulates steepness of the taper
    
    int kk;
    for(kk=0; kk<size; kk++){
        
	    alpha = al[kk];
        iota = io[kk];
        S1x = s1x[kk];
        S1y = s1y[kk];
        S1z = s1z[kk];
        S2x = s2x[kk];
        S2y = s2y[kk];
        S2z = s2z[kk];
	    Phi = Phase[kk];
        om = freq[kk];
	
	    sa = sin(alpha);
	    sa2 = sin(2.0*alpha);
	    ca = cos(alpha);
	    si = sin(iota);
	    si2 = sin(2.0*iota);
	    ci = cos(iota);
 
        a = -sn*sa;
   	    b = cs*si - sn*ci*ca;
    	c = cs*ci*ca + si*sn;

        Dx = x2*(m2/M)*S2x - x1*(m1/M)*S1x; 
        Dy = x2*(m2/M)*S2y - x1*(m1/M)*S1y; 
        Dx = x2*(m2/M)*S2z - x1*(m1/M)*S1z;

        d = Dz*sn - Dx*cs;
        

	// Computing plus polarization

  	
        C = 0.5*cs*cs*( sa*sa - ci*ci*ca*ca ) + 0.5*( ci*ci*sa*sa - ca*ca ) - 0.5*sn*sn*si*si - \
	        0.25*sn2*si2*ca;
        S = 0.5*(1.0 + cs*cs)*ci*sa2 + 0.5*sn2*si*sa;

        K = 0.5*cs*cs*(sa*sa + ci*ci*ca*ca) - 0.5*( ci*ci*sa*sa + ca*ca ) + 0.5*sn*sn*si*si + \
	        0.25*sn2*si2*ca;
 
        DC = -( Dy*sa*cs + d*ca);

        DS = -( c*Dy - d*ci*sa );

       
        Qp = -4.0*( C*cos(2.0*Phi)  + S*sin(2.0*Phi));

        P05p = 0.5*(dm/M)*( 9.0*(a*S + b*C)*cos(3.0*Phi) + 9.0*(b*S - a*C)*sin(3.0*Phi) +\
		      (3.0*a*S - 3.0*b*C - 2.0*b*K)*cos(Phi) - (3.0*b*S + 3.0*a*C - 2.0*a*K)*sin(Phi) );

        P1p = (16./3.)*(1.-3.*eta)*( ((a*a - b*b)*C - 2.*a*b*S)*cos(4.0*Phi) + \
		       ((a*a - b*b)*S + 2.*a*b*C)*sin(4.0*Phi)  ) + \
	           (1./3.)*(4.*(1.- 3.*eta)*( (b*b - a*a)*K + 2*(a*a + b*b)*C ) + 2.0*(13.- eta)*C )*cos(2.*Phi) +\
	           (1./3.)*(8.*(1.- 3.*eta)*( (a*a + b*b)*S - a*b*K ) + 2.0*(13.- eta)*S )*sin(2.*Phi) +\
	           DC*cos(Phi) + DS*sin(Phi);
       
       // Computing cross polariztion
       
        C = -0.5*cs*sa2*(1.0 + ci*ci) - 0.5*sn*si2*sa;

        S = -cs*ci*cos(2.*alpha) - sn*si*ca;

        K = -0.5*cs*sa2*si*si + 0.5*sn*si2*sa;

        DC = Dy*ca - d*cs*sa;

        DS = -Dy*ci*sa + c*d;

        Qc = -4.0*( C*cos(2.0*Phi)  + S*sin(2.0*Phi));

        P05c = 0.5*(dm/M)*( 9.0*(a*S + b*C)*cos(3.0*Phi) + 9.0*(b*S - a*C)*sin(3.0*Phi) +\
		      (3.0*a*S - 3.0*b*C - 2.0*b*K)*cos(Phi) - (3.0*b*S + 3.0*a*C - 2.0*a*K)*sin(Phi) );

        P1c = (16./3.)*(1.-3.*eta)*( ((a*a - b*b)*C - 2.*a*b*S)*cos(4.0*Phi) + \
		       ((a*a - b*b)*S + 2.*a*b*C)*sin(4.0*Phi)  ) + \
	           (1./3.)*(4.*(1.- 3.*eta)*( (b*b - a*a)*K + 2*(a*a + b*b)*C ) + 2.0*(13.- eta)*C )*cos(2.*Phi) +\
	           (1./3.)*(8.*(1.- 3.*eta)*( (a*a + b*b)*S - a*b*K ) + 2.0*(13.- eta)*S )*sin(2.*Phi) +\
	           DC*cos(Phi) + DS*sin(Phi);
 	
      
        Mom13 = pow(M*om, 1./3.);
        Mom23 = Mom13*Mom13;
        wk = 0.5*( 1.0 - tanh(Ataper*(Mom23-xmax)) );
      
        switch(order){
          case 0:
	         hp = (mu/dist)*Mom23*Qp;
	         hc = (mu/dist)*Mom23*Qc;
	         if(kk==0){
		        std::cout << "Only main harmonic" << std::endl;
	         }
            break;
          case 1:
             hp = (mu/dist)*Mom23*Mom13*P05p;
             hc = (mu/dist)*Mom23*Mom13*P05c;
	         if(kk==0){
		         std::cout << "Only 0.5PN ampl" << std::endl;
	         }
            break;
          case 2:
             hp = (mu/dist)*Mom23*Mom23*P1p;
             hc = (mu/dist)*Mom23*Mom23*P1c;
	         if(kk==0){
		        std::cout << "Only 1PN ampl" << std::endl;
	         }
	        break;
	      default:
	         hp = (mu/dist)*Mom23*( Qp + Mom13*P05p + Mom23*P1p);
             hc = (mu/dist)*Mom23*( Qc + Mom13*P05c + Mom23*P1c);
	         if(kk==0){
		          std::cout << "The whole waveform" << std::endl;
	         }
	        break;  
        }
        if (taper != 0.0){
           hp *= wk;
           hc *= wk;
        }
        *hPlus  =  hp*cos(2.0*psi) + hc*sin(2.0*psi);
        *hCross = -hp*sin(2.0*psi) + hc*cos(2.0*psi);
        hPlus++;
        hCross++;
 
    }
   
    return((int)size);

}



}// end of the namespace
