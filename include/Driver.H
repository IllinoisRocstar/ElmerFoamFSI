/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Example program interface
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// 
#ifndef __ELMER_FOAM_FSI_H__
#define __ELMER_FOAM_FSI_H__
#ifdef _ELMERFOAMFSI_PARALLEL_
#include "COMM.H"
#endif
#include "Global.H"
#include "Profiler.H"
#include "com.h"

namespace ElmerFoamFSI {
  
  ///
  /// Encapsulate example program-specific code constructs.
  ///
  /// This namespace contains all of the base code constructs
  /// that are used in the example programs.  These example
  /// programs have several features which make them very 
  /// easy to integrate in composite software systems. 
  ///
  /// * Program is entirely encapsulated in program object
  /// * All screen output is automatically tagged with the program name
  /// * Common look-and-feel interface
  /// * Interface functionality allows menu-driven integration
  ///
  /// Other convenient features of this API are:
  ///
  /// * Built-in stack tracing for debugging
  /// * Built-in profiling utilities
  /// * Advanced command-line handling
  /// * Built-in parallelism 
  ///
  //  namespace ExampleProgram {
  ///
  /// Convenience type definition for Profiler object.
  ///
  typedef IRAD::Profiler::ProfilerObj ProfilerType;
  ///
  /// Convenience type definition for program stack.
  ///
  typedef std::string                 StackType;
  ///
  /// Convenience type definition for the serial global object.
  ///
  /// The global object encapsulates all of the built-in functionalities
  /// for a program.  Its interface and implementation are defined in 
  /// IRAD/include/Global.H 
  ///
  typedef IRAD::Global::GlobalObj<StackType,int,ProfilerType> GlobalType;
#ifdef _ELMERFOAMFSI_PARALLEL_
  ///
  /// Convenience type definition for parallel communication object.
  ///
  typedef IRAD::Comm::CommunicatorObject CommType;
  ///
  /// Convenience type definition for parallel global object
  ///
  typedef IRAD::Global::ParallelGlobalObj<CommType,StackType,int,ProfilerType> PGlobalType;
#endif
  ///
  /// The ComLineObject for the example programs.
  ///
  /// This comline object implements a basic command-line interface
  /// handler for the example programs.  These command-line objects
  /// handle all of the generation of help messages, command line 
  /// argument validation, and encapsulation.
  ///
  class DriverComLine : public IRAD::Util::ComLineObject
  {
  public:
    DriverComLine()
      : IRAD::Util::ComLineObject()
    {};
    DriverComLine(const char *args[])
      : IRAD::Util::ComLineObject(args)
    {};
    ///
    /// This is where the actual options and arguments are described.
    ///
    /// For details about what can be done in this function, see the
    /// documentation for the IRAD::Util::ComLineObject.
    ///
    void Initialize(){
      AddOption('h',"help");
      AddOption('d',"debug");
      AddOption('v',"verbosity",1);
      AddOption('o',"output",2,"filename");
      //      AddArgument("input",1);
      AddHelp("help","Prints this long version of help.");
      AddHelp("debug","Turns on debugging output.");
      AddHelp("output","Specifies the name of the output file.");
      AddArgument("inputfile",1);
      std::ostringstream Ostr;
      Ostr << "A configuration file with parameters.";
      // Ostr << "Mode-dependent input: Input <filename> for serial example"
      //      << "\n\t\tprogram, or <number of divisions> for parallel example.";
      AddArgHelp("inputfile",Ostr.str());
      // The following commented string formation is kept around to 
      // illustrate the required tabs and newlines to make the output 
      // look proper:
      //
      //    Ostr << "Use fixed problem size in scalability analysis.  Only makes"
      //         << "\n\t\tsense when scalability mode is enabled.";
      //    Ostr.str("");
      Ostr.str("");
      Ostr << "Example ElmerFoamFSI program.";
      _description.assign(Ostr.str());
    };
  };
    
  ///
  /// Convenience type definition for the example comline object.
  ///
  typedef DriverComLine ComLineType;
  ///
  /// Convenience type definition for the serial program.
  /// 
  /// For a detailed description of the program type, see IRAD::Global::Program
  /// documentation.
  ///
  typedef IRAD::Global::Program<GlobalType,ComLineType> SerialProgramType;
#ifdef _ELMERFOAMFSI_PARALLEL_
  ///
  /// Convenience type definition for the parallel program.
  /// 
  /// For a detailed description of the program type, see IRAD::Global::Program
  /// documentation.
  ///
  typedef IRAD::Global::Program<PGlobalType,ComLineType> ParallelProgramType;
#endif
  ///
  /// Implementation of the basic parts of the serial program example.
  ///
  /// This object encapsulates the example serial program.  It inherits
  /// from the IRAD::Global::Program type.  
  ///
  /// The program itself just copies a specified input file to the 
  /// specified output file (or stdout if no file is given). The 
  /// command line usage goes:
  /// 
  ///
  ///            elmerfsidriver Usage:
  /// 
  ///            elmerfsidriver [-h] [-v [arg] -o <filename> ] <input> 
  ///
  ///            -h,--help
  ///               Prints this long version of help.
  ///
  ///                   -v,--verbosity [arg]
  /// 
  ///            -o,--output <filename>
  ///               Specifies the name of the output file.
  ///
  ///                   <input>
  ///               Mode-dependent input: input <filename> for serial example
  ///               program, or <number of divisions> for parallel example.
  ///
  class SerialDriverProgram : public SerialProgramType
  {
  protected:
    /// Name of file for output.
    std::string output_name;
    /// Name of input file.
    std::string input_name;
    /// Verbosity level. 
    int verblevel;
    /// Outfile stream for output.
    std::ofstream Ouf;
    /// Infile stream for input.
    std::ifstream Inf;
    /// Handle/identifier for fluids init
    int fluidsInitHandle;
    /// Handle/identifier for fluids run
    int fluidsRunHandle;
    /// Handle/identifier for fluids step
    int fluidsStepHandle;
    /// Handle/identifier for fluids finalize
    int fluidsFinalizeHandle;
    /// Handle/identifier for structures init
    int structuresInitHandle;
    /// Handle/identifier for structures run
    int structuresRunHandle;
    /// Handle/identifier for structures finalize
    int structuresFinalizeHandle;
    /// Object-local copy for argc
    int argc;
    /// Object-local copy for argv
    char **argv;
  public:
    ///
    /// Default constructor.
    ///
    SerialDriverProgram() :
      SerialProgramType()
    {};
    ///
    /// Constructor designed to take the commandline args.
    /// 
    SerialDriverProgram(int nargs,char **args) :
      SerialProgramType(nargs,args), argc(nargs), argv(args)
    {
      int nargs_dupe = 2;
      char **args_dupe;
      args_dupe = new char * [3];
      args_dupe[0] = const_cast<char *>("test");
      args_dupe[1] = const_cast<char *>("-com-mpi");
      args_dupe[2] = NULL;
      COM_init(&nargs_dupe,&args_dupe);
      COM_set_profiling(1);
    };
    ///
    /// Initializes native data structures from commandline args.
    /// 
    virtual int Initialize()
    {
      int retval = SerialProgramType::Initialize();
      if(!_command_line.GetOption("help").empty()){
        std::ostringstream Ostr;
        Ostr << _command_line.LongUsage() << std::endl;
        StdOut(Ostr.str());
        return(-1);
      }
      if(retval){
        std::ostringstream Ostr;
        Ostr << _command_line.ErrorReport() << std::endl
             << std::endl << _command_line.ShortUsage() << std::endl;
        ErrOut(Ostr.str());
        return(retval);
      }
      if(!_command_line.GetOption("debug").empty()){
        this->SetDebugLevel(2);
        this->SetDebugStream(std::cout);
      }
      // Check if output file is specified
      output_name = _command_line.GetOption("output");
      
      // Set input file to first argument
      std::vector<std::string> args(_command_line.GetArgs());
      input_name = args[0];
      
      // Check the verbosity level
      std::string sverb(_command_line.GetOption("verbosity"));
      if(sverb.empty() || sverb == ".true.")
        verblevel = 1;
      else {
        std::istringstream Vin(sverb);
        Vin >> verblevel;
        if(verblevel < 0)
          verblevel = 1;
      }
     
      //Set the verblevel within IRAD
      SetVerbLevel((char)verblevel);

      // If high verbosity, stick a configuration blurb to stdout.
      if(verblevel > 1){
        std::ostringstream Ostr;
        Ostr << "Configuration:" << std::endl
             << "verbosity           = " << verblevel << std::endl
             << "config file         = " << input_name << std::endl;
        if(!output_name.empty())
          Ostr << "output file         = " << output_name << std::endl;
        Ostr << std::endl;
        StdOut(Ostr.str());
      } 
      
      return(0);
    };
    ///
    /// Returns verbosity level.
    ///
    int VerbLevel() const { return verblevel;};
    ///
    /// Destructor
    ///
    virtual ~SerialDriverProgram() {
      if(COM_initialized()) {
        COM_print_profile("run_profile.dat", "");
        COM_finalize();
      }
      Report(Ouf);
      if(Ouf){
        Ouf.close();
        SetOutStream(std::cout);
      }
    };
    ///
    /// This function implements the main function executed by the program.
    /// 
    int DriverRun();
  };
    

#ifdef _ELMERFOAMFSI_PARALLEL_
  ///
  /// Implementation of the basic parts of the parallel program example.
  ///
  /// This object encapsulates the example parallel program.  It inherits
  /// from the IRAD::Global::Program type. 
  /// 
  /// The program itself calculates \f$\pi\f$ by integrating 
  /// \f$\frac{d}{dx}(4\,\arctan(x))\f$ over the interval [0,1]:
  ///
  /// \f$\pi = \int_{0}^{1}\frac{4}{(1+x^2)}{dx}\f$
  ///
  /// This method is MPI parallel and uses domain decomposition to 
  /// calculate the total integral. On _N_ processors, the 
  /// _n_ partitions of the domain, [0,1], are partitioned as follows:
  ///
  /// First, the domain is broken into _n_ equal pieces of size _h_:
  ///
  /// \f$h = \frac{1}{n}\f$
  ///
  /// Each processor gets a roughly equal number of pieces, \f$n_i\f$. 
  /// The domain on each processor,\f$\mathcal{D}_i\f$, is defined as:
  ///
  /// \f$\mathcal{D}_i \equiv [x_i,x_{i+1}],\;\forall\,i \in [0,N-1]\f$
  ///
  /// With \f$x_0 = 0\f$ and \f$x_i = x_{i-1} + hn_{i-1},\;\forall\, i > 0\f$, 
  /// each processor performs the integration on its own portion of the domain:
  ///
  /// \f$\mathcal{I}_i = \int_{\mathcal{D}_i}\frac{4}{(1+x^2)}{dx}\f$ 
  /// 
  /// The approximate values for the integrals on each processor,
  /// \f$\bar{\mathcal{I}_i}\f$, are determined by numerical quadrature
  /// using two different methods:\n
  ///
  /// 1. ElmerFoamFSI::TrapezoidQuadrature 
  /// 2. ElmerFoamFSI::MidPointQuadrature
  ///
  /// The results of these quadrature methods from each processors 
  /// are then summed to recover a total numerical 
  /// estimate for \f$\pi\f$:
  ///
  /// \f$\bar{\pi} = \sum_{i=0}^{N-1}\bar{\mathcal{I}_i}\f$
  ///
  /// The approximate value, \f$\bar{\pi}\f$ and the error, 
  /// \f$|\pi-\bar{\pi}|\f$ are reported to stdout or, if specified, to
  /// a file.
  ///
  /// @note If the number of partitions, _n_, is less than the number
  /// of processors, _N_, then the program behaves as if it were serial,
  /// performing all calculations on processor 0.
  ///
  /// The usage for this program goes like:
  ///               
  ///           pepi Usage: 
  /// 
  ///           pepi [-h] [-v [arg] -o <filename> ] <input> 
  /// 
  ///           -h,--help
  ///              Prints this long version of help.
  ///        
  ///                  -v,--verbosity [arg]
  ///        
  ///                  -o,--output <filename>
  ///                     Specifies the name of the output file.
  ///
  ///           <input>
  ///                     Mode-dependent input: Input <filename> for serial example
  ///                     program, or <number of divisions> for parallel example.
  ///
  class ParallelDriverProgram : public ParallelProgramType
  {
  protected:
    /// Name of file for output.
    std::string output_name;
    /// Name of input file.
    std::string input_name;
    /// Verbosity level. 
    int verblevel;
    /// Outfile stream for output.
    std::ofstream Ouf;
    /// Infile stream for input.
    std::ifstream Inf;
    /// Handle/identifier for fluids init
    int fluidsInitHandle;
    /// Handle/identifier for fluids run
    int fluidsRunHandle;
    /// Handle/identifier for fluids step
    int fluidsStepHandle;
    /// Handle/identifier for fluids finalize
    int fluidsFinalizeHandle;
    /// Handle/identifier for structures init
    int structuresInitHandle;
    /// Handle/identifier for structures run
    int structuresRunHandle;
    /// Handle/identifier for structures finalize
    int structuresFinalizeHandle;
    /// Object-local copy for argc
    int argc;
    /// Object-local copy for argv
    char **argv;
  public:
    /// 
    /// Default constructor.
    ///
    ParallelDriverProgram() :
      ParallelProgramType()
    {verblevel = 0;};
    /// 
    /// Constructor designed to take all commandline args.
    ///
    ParallelDriverProgram(int nargs,char **args) :
      ParallelProgramType(nargs,args), argc(nargs), argv(args)
    {
       // initializing COM with -com-mpi to force calling MPI_init
       int nargs_dupe = 2;
       char **args_dupe;
       args_dupe = new char * [3];
       args_dupe[0] = const_cast<char *>("dummy");
       args_dupe[1] = const_cast<char *>("-com-mpi");
       args_dupe[2] = NULL;
       COM_init(&nargs_dupe,&args_dupe);
       COM_set_profiling(1);
       // at this point we should have communicator setup
       //std::cout << "My Rank = " << Rank() << std::endl;
    };
    /// 
    /// Constructor designed to take all commandline args.
    ///
    ParallelDriverProgram(ComLineType &comline,CommType &incomm) 
    {
      this->_command_line.Copy(comline);
      this->Init(_command_line.ProgramName(),incomm);
      verblevel = 0;
    };
    ///
    /// Populates native data structures from commandline.
    ///
    virtual int Initialize()
    {
      int retval = ParallelProgramType::Initialize();
      // Check if the user just entered -h or --help
      if(!_command_line.GetOption("help").empty()){
        std::ostringstream Ostr;
        Ostr << _command_line.LongUsage() << std::endl;
        StdOut(Ostr.str());
        return(-1);
      }
      if(retval){ // then there were errors on the command line
        std::ostringstream Ostr;
        Ostr << _command_line.ErrorReport() << std::endl
             << std::endl << _command_line.ShortUsage() << std::endl;
        ErrOut(Ostr.str());
        return(retval);
      }
      // setting up debuger if needed
      if(!_command_line.GetOption("debug").empty()){
        this->SetDebugLevel(2);
        this->SetDebugStream(std::cout);
      }

      // Everything is OK up to here, so check for an output file.
      output_name = _command_line.GetOption("output");

      // Set input file to first argument
      std::vector<std::string> args(_command_line.GetArgs());
      input_name = args[0];

      // Check the verbosity level
      std::string sverb(_command_line.GetOption("verbosity"));
      if(sverb.empty() || sverb == ".true.")
        verblevel = 1;
      else {
        std::istringstream Vin(sverb);
        Vin >> verblevel;
        if(verblevel < 0)
          verblevel = 1;
      }

      //Set the verblevel within IRAD
      SetVerbLevel((char)verblevel);

      // If high verbosity, stick a configuration blurb to stdout.
      if(verblevel > 1){
        std::ostringstream Ostr;
        Ostr << "*****************************************************"<< std::endl;
        Ostr << "*            Reading the input file                 *"<< std::endl;
        Ostr << "*****************************************************"<< std::endl;
        Ostr << "* Configuration:" << std::endl
             << "* verbosity           = " << verblevel << std::endl;
        if(!input_name.empty())
          Ostr << "* input file          = " << input_name << std::endl;
        if(!output_name.empty())
          Ostr << "* output file         = " << output_name << std::endl;
        Ostr << "*" << std::endl;
        Ostr << "*****************************************************"<< std::endl;
        StdOut(Ostr.str());
      } 

      return(0);
    };
    ///
    /// Returns verbosity level.
    ///
    int VerbLevel() const { return verblevel;};
    ///
    /// Destructor.
    ///
    virtual ~ParallelDriverProgram() {
      if(COM_initialized()) {
        COM_print_profile("run_profile.dat", "");
        COM_finalize();
      }
      if(Ouf){
        Ouf.close();
        SetOutStream(std::cout);
      }
    };
    ///
    /// This function implements the main function executed by the program.
    /// 
    int DriverRun();
  };
#endif    


  ///
  /// Unified driver for the example programs
  ///
  /// This driver is exactly the same for both 
  /// the serial and parallel example programs.
  /// It is a direct replacement for main and
  /// is responsible for creating the "Program object"
  /// and calling its Initialization, Run, and 
  /// Finalize methods.
  ///
  template<typename ProgramType>
  int Driver(int argc,char *argv[])
  {
    ProgramType MyProgram(argc,argv);
    int retval = MyProgram.Initialize();
    if(retval){
      if(retval > 0){ // then there was an error
        MyProgram.ErrOut("Error in Program Initialization.\n");
        return(1);
      }
      else return(0); // no error, but the program should not continue
    }
    if(MyProgram.VerbLevel() > 1)
      MyProgram.StdOut("Calling RUN.\n");
    if(MyProgram.DriverRun()){
      MyProgram.ErrOut("Error in Program RUN method.\n");
      return(1);
    }
    if(MyProgram.VerbLevel() > 1){
      std::ostringstream Ostr;
      MyProgram.Report(Ostr);
      MyProgram.StdOut(Ostr.str());
      MyProgram.StdOut("Calling Finalize.\n");
    }
    if(MyProgram.Finalize()){
      MyProgram.ErrOut("Error in Program Finalization.\n");
      return(1);
    }
    if(MyProgram.VerbLevel() > 1)
      MyProgram.StdOut("All done.\n");
    return(0);
  }


  ///
  /// Convenience definition for serial program type
  ///
  typedef ElmerFoamFSI::SerialDriverProgram DriverProgramType;
#ifdef _ELMERFOAMFSI_PARALLEL_
  /// Support function for parallel example. 
  ///
  /// @param x The x-coordinate.
  /// @returns The value of the function at x
  ///
  /// This function returns the derivative of
  /// (4 arctan(x)) at x.  Specifically, it implements:
  ///
  /// \f$f(x) = \frac{d}{dx}({4}\arctan(x)) = \frac{4}{(1+x^2)}\f$.
  /// 
  inline double f(double x)
  {
    return (4.0 / (1.0 + x*x));
  }
  ///
  /// Convenience definition for parallel program type.
  ///
  typedef ElmerFoamFSI::ParallelDriverProgram PEProgramType;
#endif
  //  };
};
#endif
