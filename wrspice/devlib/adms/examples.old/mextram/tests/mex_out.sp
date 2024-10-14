MEXTRAM Output Test Ic=f(Vc,Ib)

* SRW  This circuit tends to have difficulty converging to a DC
* operating point.  What tends to happen is that the matrix elements
* grow so large when iterating and not converging that the matrix
* becomes singualar and the run aborts.  However, if we limit the
* number of iterations, the trial will simply fail, a smaller step
* will be attempted, and this will converge.  The gmin stepping works
* with default values, however source stepping, which by default is
* tried first, doesn't.  So, we set the option to do gmin stepping
* first below.
*
.options gminfirst

* One could alternatively fix source stepping with
* .options itl2src=10

IB 0 b 1u
VC C 0 2.0
VS S 0 0.0
Q1 C B 0 S DT BJTRF1

.control
dc vc 0.0 6.0 0.05 ib 1u 8u 1u
plot abs(i(vc)) xlabel Vce title Output-Characteristic
plot v(dt) xlabel Vce ylabel grdC title Selfheating
.endc

.model BJTRF1 NPN LEVEL=6
+MULT=1.000E+00
+TREF=25.000E+00
+DTA=0.000E+00
+EXMOD=1.000E+00
+EXPHI=0.000E+00
+EXAVL=1.000E+00
+IS=23.571E-18
+IK=231.660E-03
+VER=2.100E+00
+VEF=36.001E+00
+BF=186.538E+00
+IBF=1.140E-15
+MLF=2.000E+00
+XIBI=0.000E+00
+BRI=9.231E+00
+IBR=61.600E-15
+VLR=400.000E-03
+XEXT=648.148E-03
+WAVL=1.064E-06
+VAVL=3.330E+00
+SFH=882.471E-03
+RE=949.668E-03
+RBC=27.769E+00
+RBV=32.004E+00
+RCBLX=10.0
+RCBLI=10.0
+RCC=18.026E+00
+RCV=237.417E+00
+SCRCV=882.839E+00
+IHC=3.370E-03
+AXI=300.000E-03
+CJE=55.566E-15
+VDE=900.000E-03
+PE=500.000E-03
+XCJE=52.478E-03
+CJC=25.153E-15
+VDC=660.000E-03
+PC=450.000E-03
+XP=310.000E-03
+MC=500.000E-03
+XCJC=122.100E-03
+MTAU=1.000E+00
+TAUE=6.200E-12
+TAUB=977.273E-15
+TEPI=7.980E-12
+TAUR=64.400E-12
+DEG=0.000E+00
+XREC=0.000E+00
+AQBO=701.246E-03
+AE=308.246E-03
+AB=846.000E-03
+AEPI=2.500E+00
+AEX=619.000E-03
+AC=1.580E+00
+DVGBF=52.000E-03
+DVGBR=0.000E+00
+VGB=1.197E+00
+VGC=1.200E+00
+VGJ=1.200E+00
+DVGTE=1.202E+00
+AF=2.350E+00
+KF=47.298E-09
+KFN=1.000E-09
+ISS=18.480E-18
+IKS=219.348E-06
+CJS=146.628E-15
+VDS=542.048E-03
+PS=314.095E-03
+VGS=1.221E+00
+AS=1.580E+00
+RTH=300
+CTH=3E-09

.end
