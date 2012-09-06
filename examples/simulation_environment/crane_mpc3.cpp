/*
 *    This file is part of ACADO Toolkit.
 *
 *    ACADO Toolkit -- A Toolkit for Automatic Control and Dynamic Optimization.
 *    Copyright (C) 2008-2009 by Boris Houska and Hans Joachim Ferreau, K.U.Leuven.
 *    Developed within the Optimization in Engineering Center (OPTEC) under
 *    supervision of Moritz Diehl. All rights reserved.
 *
 *    ACADO Toolkit is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    ACADO Toolkit is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with ACADO Toolkit; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */



 /**
 *    \file crane_simulation.cpp
 *    \author Boris Houska, Hans Joachim Ferreau
 *    \date 2008
 */



#include <acado_toolkit.hpp>
#include <include/acado_gnuplot/gnuplot_window.hpp>


int main( ){

    USING_NAMESPACE_ACADO;

    // VARIABLES:
    // ------------------------
    DifferentialState        x;   // Position of the trolley
    DifferentialState        v;   // Velocity of the trolley
    DifferentialState      phi;   // excitation angle
    DifferentialState     dphi;   // rotational velocity

	Control 				ax;   // trolley accelaration
	Disturbance 			 W;   // disturbance

    double L = 1.0 ;              // length
	double m = 1.0 ;              // mass
	double g = 9.81;              // gravitational constant
	double b = 0.2 ;              // friction coefficient


    // DIFFERENTIAL EQUATION:
    // ------------------------
    DifferentialEquation     f, fSim;   // The model equations

    f << dot(x) ==  v;
    f << dot(v) ==  ax;
    f << dot(phi ) == dphi;
    f << dot(dphi) == -g/L*sin(phi) -ax/L*cos(phi) - b/(m*L*L)*dphi;

	L = 1.2;							// introduce model plant mismatch
	
	fSim << dot(x) ==  v;
	fSim << dot(v) ==  ax + W;
	fSim << dot(phi ) == dphi;
	fSim << dot(dphi) == -g/L*sin(phi) -ax/L*cos(phi) - b/(m*L*L)*dphi;
	

    // DEFINE LEAST SQUARE FUNCTION:
    // -----------------------------
    Function h;

    h << x;
    h << v;
    h << phi;
    h << dphi;

    Matrix Q(4,4); // LSQ coefficient matrix
    Q.setIdentity();

    Vector r(4); // Reference


    // DEFINE AN OPTIMAL CONTROL PROBLEM:
    // ----------------------------------
    const double t_start = 0.0;
    const double t_end   = 5.0;

    OCP ocp( t_start, t_end, 25 );

    ocp.minimizeLSQ( Q, h, r );
    ocp.subjectTo( f );
    ocp.subjectTo( -5.0 <= ax <= 5.0 );


    // SETTING UP THE (SIMULATED) PROCESS:
    // -----------------------------------
	OutputFcn identity;
	DynamicSystem dynamicSystem( fSim,identity );

	Process process( dynamicSystem,INT_RK45 );

	VariablesGrid disturbance = readFromFile( "dist.txt" );
	process.setProcessDisturbance( disturbance );

	
    // SETTING UP THE MPC CONTROLLER:
    // ------------------------------

	RealTimeAlgorithm alg( ocp,0.1 );
//  	alg.set( USE_REALTIME_ITERATIONS,NO );
//  	alg.set( MAX_NUM_ITERATIONS,20 );

	StaticReferenceTrajectory zeroReference;

	Controller controller( alg,zeroReference );
	

    // SETTING UP THE SIMULATION ENVIRONMENT,  RUN THE EXAMPLE...
    // ----------------------------------------------------------
	SimulationEnvironment sim( 0.0,20.0,process,controller );

	Vector x0(4);
	x0.setZero();
	x0(3) = 5.0;

	sim.init( x0 );
	sim.run( );


    // ...AND PLOT THE RESULTS
    // ----------------------------------------------------------
	VariablesGrid diffStates;
	sim.getProcessDifferentialStates( diffStates );

	VariablesGrid feedbackControl;
	sim.getFeedbackControl( feedbackControl );

	GnuplotWindow window;
		window.addSubplot( diffStates(0),   "POSITION OF THE TROLLEY" );
		window.addSubplot( diffStates(1),   "VELOCITY OF THE TROLLEY" );
		window.addSubplot( diffStates(2),   "PHI" );
		window.addSubplot( diffStates(3),   "DPHI" );
		window.addSubplot( feedbackControl, "Accelaration [m/s^2]" );
	//	window.addSubplot( disturbance,     "Disturbance [m/s^2]" );
	window.plot();
	
	diffStates.printToFile( "diffStates.txt" );


    return 0;
}

/* <<< end tutorial code <<< */
