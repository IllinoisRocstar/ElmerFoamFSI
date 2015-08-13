#ifndef __ORCHESTRATOR_H__
#define __ORCHESTRATOR_H__

namespace impact {
  namespace orchestrator {

    class agentbase {
    public:
      enum {INIT=0,INITTIME,RUN,FINALIZETIME,FINALIZE,READ,WRITE,SET,OP1,OP2,OP3,NUMBER_OF_FUNCTIONS};
    protected:
      std::vector<std::string> componentInterfaceNames;
      std::vector<std::string> modelInterfaceNames;
      std::vector<int> componentFunctionHandles;
      std::ostream *errStream;
      std::ostream *outStream;
    public:
      agentbase() : errStream(NULL), outStream(NULL) {
        componentFunctionHandles.resize(NUMBER_OF_FUNCTIONS,0);
      };
      virtual void SetErrStream(std::ostream &oStream){ errStream = &oStream; };
      virtual void SetOutStream(std::ostream &oStream){ outStream = &oStream; };
      virtual int Initialize(const std::string &interfaceName){return(0);};
      virtual int Run(){return(0);};
      virtual int InitializeTimeStep(double time){return(0);};
      virtual int FinalizeTimeStep(double time){return(0);};
      virtual int Finalize(){return(0);};
      virtual int DumpSolution(){return(0);};
      virtual void WriteOut(const std::string &output){
        if(outStream)
          *outStream << output;
      }
      virtual void WriteErr(const std::string &output){
        if(errStream)
          *errStream << output;
      }
      virtual ~agentbase(){};
    };

    class couplingbase {
    public:
      enum {TIME=0,DT,START_TIME,END_TIME,INTERVAL1,INTERVAL2,NUMBER_OF_REALS};
      enum {STEP,MAX_STEP,FREQUENCY1,FREQUENCY2,NUMBER_OF_INTEGERS};
    protected:
      std::vector<impact::orchestrator::agentbase *> componentAgents;
      std::vector<double> realNumbers;
      std::vector<int> intNumbers;
      std::ostream *errStream;
      std::ostream *outStream;
    public:
      couplingbase() : errStream(NULL), outStream(NULL) {
        realNumbers.resize(NUMBER_OF_REALS,0);
        intNumbers.resize(NUMBER_OF_INTEGERS,0);
      }
      virtual void SetErrStream(std::ostream &oStream){ errStream = &oStream; };
      virtual void SetOutStream(std::ostream &oStream){ outStream = &oStream; };
      virtual int Initialize(std::vector<std::string> &componentInterfaceNames){return(0);};
      virtual int Run(){return(0);};
      virtual int Finalize(){return(0);};
      virtual int DumpSolution(){return(0);};
      virtual void WriteOut(const std::string &output){
        if(outStream)
          *outStream << output;
      }
      virtual void WriteErr(const std::string &output){
        if(errStream)
          *errStream << output;
      }
      virtual ~couplingbase(){};
      
    };
  }
}

#endif
