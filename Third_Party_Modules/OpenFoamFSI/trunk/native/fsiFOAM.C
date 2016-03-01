#include "fsiFOAM.H"

using namespace COM;

// my includes
//using namespace std;

int fsifoam_module::Initialize(int argc,char *argv[])
{
  sumLocalContErr = 0;
  globalContErr = 0;
  cumulativeContErr = 0;
  pRefCell = 0;
  pRefValue = 0.0;

  argsPtr = new Foam::argList(argc,argv);
  if(!argsPtr){
    Info << "FsiFoam:Initialize: Error: Failed to create args object." << endl;
    return(1);
  }
  if (!argsPtr->checkRootCase()){
    Info << "FsiFoam:Initialize: Error: Root case setup failed." << endl;
    return(1);
  }
  Foam::argList &args(*argsPtr);    
  runTimePtr = new Foam::Time
    (
     Foam::Time::controlDictName,
     args.rootPath(),
     args.caseName(),
     "system",
     "constant",
     !args.optionFound("noFunctionObjects")
     );
  if(!runTimePtr){
    Info << "FsiFoam:Initialize: Error: Failed to setup time object." << endl;
    return(1);
  }

  InitFluidMesh();
  InitTransportProperties();
  CreateFluidFields();
  InitStructuresMesh();
  CreateStructuresFields();
  ReadCouplingProperties();
  CreateInterZoneInterpolators();
  FindGlobalFaceZones();

  // functions specific to IMPACT FSI
  CreateFSISurfaceMesh();

  return(0);
};

int fsifoam_module::InitFluidMesh(){
  //    Foam::argList &args(*argsPtr);    
  Foam::Time &runTime(*runTimePtr);
  Info << "FsiFoam:Initialize: Create dynamic mesh for time = "
       << runTime.timeName() << endl;
  //    Info << OutStream.str() << endl;
  //    OutStream.clear();
  //    OutStream.str("");
  
  meshPtr = dynamicFvMesh::New(IOobject(dynamicFvMesh::defaultRegion,
                                        runTime.timeName(),
                                        runTime,
                                        IOobject::MUST_READ));
  return(0);
};

int fsifoam_module::InitTransportProperties(){
  Foam::Time &runTime(*runTimePtr);
  dynamicFvMesh &mesh = meshPtr();
  Info << "FsiFoam:Initialize: Reading transportProperties" << endl;
  transportPropertiesPtr = new IOdictionary(IOobject("transportProperties",
                                                     runTime.constant(),
                                                     mesh,
                                                     IOobject::MUST_READ,
                                                     IOobject::NO_WRITE));
  return(0);
};

int fsifoam_module::CreateFluidFields(){
  //    Foam::argList &args(*argsPtr);    
  Foam::Time &runTime(*runTimePtr);
  dynamicFvMesh &fluidsMesh = meshPtr();
  IOdictionary &transportProperties(*transportPropertiesPtr);
  
  
  nuPtr = new dimensionedScalar(transportProperties.lookup("nu"));
  rhoFluidPtr = new dimensionedScalar(transportProperties.lookup("rho"));
  
  Info << "FsiFoam:CreateFluidFields: Reading field p" << endl;
  // Info << OutStream.str() << endl;
  // OutStream.clear();
  // OutStream.str("");
  
  pPtr = new volScalarField(IOobject("p",
                                     runTime.timeName(),
                                     fluidsMesh,
                                     IOobject::MUST_READ,
                                     IOobject::AUTO_WRITE),
                            fluidsMesh);
  
  volScalarField &p(*pPtr);
  
  Info << "FsiFoam:CreateFluidFields: Reading field U" << endl;
  // Info << OutStream.str() << endl;
  // OutStream.clear();
  // OutStream.str("");
  
  UPtr = new volVectorField(IOobject("U",runTime.timeName(),fluidsMesh,
                                     IOobject::MUST_READ,IOobject::AUTO_WRITE),fluidsMesh);
  
  volVectorField &U = *UPtr;
  
  //#   include "createPhi.H"
  Info << "FsiFoam:CreateFluidFields: Reading/calculating face flux field phi" << endl;
  
  phiPtr = new surfaceScalarField(IOobject("phi",runTime.timeName(),fluidsMesh,
                                           IOobject::READ_IF_PRESENT,IOobject::AUTO_WRITE),
                                  linearInterpolate(U) & fluidsMesh.Sf());
  
  
  
  setRefCell(p, fluidsMesh.solutionDict().subDict("PISO"), pRefCell, pRefValue);
  return(0);
};

int fsifoam_module::InitStructuresMesh(){
  Foam::Time &runTime(*runTimePtr);
  stressMeshPtr = new fvMesh(IOobject("solid",runTime.timeName(),runTime,IOobject::MUST_READ));
  return(stressMeshPtr != NULL);
};

int fsifoam_module::CreateStructuresFields(){
  Foam::Time &runTime(*runTimePtr);
  fvMesh &structMesh(*stressMeshPtr);
  
  Info << "FsiFoam:CreateStructuresFields: Reading incremental displacement field DU" << endl;
  // Info << OutStream.str() << endl;
  // OutStream.clear();
  // OutStream.str("");
  
  DUPtr = new volVectorField(IOobject("DU",runTime.timeName(),structMesh,
                                      IOobject::MUST_READ,IOobject::AUTO_WRITE),
                             structMesh);
  
  volVectorField &DURef(*DUPtr);
  
  //    Info << "DU(0): " << DU << endl;
  
  gradDUPtr = new volTensorField(fvc::grad(DURef));
  volTensorField &gradDURef(*gradDUPtr);
  
  UsolidPtr = new volVectorField(IOobject("Usolid",runTime.timeName(),structMesh,
                                          IOobject::READ_IF_PRESENT,IOobject::AUTO_WRITE),
                                 DURef);
  
  
  Info << "FsiFoam:CreateStructuresFields: Reading incremental displacement field DV" << endl;
  DVPtr = new volVectorField(IOobject("DV",runTime.timeName(),structMesh,
                                      IOobject::READ_IF_PRESENT,IOobject::AUTO_WRITE),
                             fvc::ddt(DURef));
  
  
  Info << "FsiFoam:CreateStructuresFields: Reading accumulated velocity field V" << endl;
  VsPtr = new volVectorField(IOobject("Vs",runTime.timeName(),structMesh,
                                      IOobject::READ_IF_PRESENT,IOobject::AUTO_WRITE),
                             structMesh,
                             dimensionedVector("zero",dimVelocity,vector::zero));
  
  
  Info << "FsiFoam:CreateStructuresFields: Reading accumulated stress field sigma" << endl;
  sigmaPtr = new volSymmTensorField(IOobject("sigma",runTime.timeName(),structMesh,
                                             IOobject::READ_IF_PRESENT,IOobject::AUTO_WRITE),
                                    structMesh,
                                    dimensionedSymmTensor("zero",dimForce/dimArea,symmTensor::zero));
  volSymmTensorField &sigmaref(*sigmaPtr);
  
  
  Info << "FsiFoam:CreateStructuresFields: Reading incremental stress field DSigma" << endl;
  DSigmaPtr = new volSymmTensorField(IOobject("DSigma",runTime.timeName(),structMesh,
                                              IOobject::READ_IF_PRESENT,IOobject::AUTO_WRITE),
                                     structMesh,
                                     dimensionedSymmTensor("zero",dimForce/dimArea,symmTensor::zero));
  
  
  rheologyPtr = new constitutiveModel(sigmaref,DURef);
  
  constitutiveModel &rheology(*rheologyPtr);
  
  //    volScalarField &rhoTest(rheology.rho());
  
  rhoPtr = new volScalarField(rheology.rho());
  muPtr  = new volScalarField(rheology.mu());
  lambdaPtr = new volScalarField(rheology.lambda());
  
  FPtr = new volTensorField(I + gradDURef.T());
  volTensorField &FRef(*FPtr);
  DFPtr = new volTensorField(FRef - I);
  JPtr = new volScalarField(det(FRef));
  
  solidDdtSchemePtr = new word(structMesh.schemesDict().ddtScheme("ddt("+DURef.name()+')'));
  return(0);
};
int fsifoam_module::ReadCouplingProperties(){
  Foam::argList &args(*argsPtr);    
  Foam::Time &runTime(*runTimePtr);
  dynamicFvMesh &fluidMesh(meshPtr());
  fvMesh &structMesh(*stressMeshPtr);
  
  Info << "FsiFoam:ReadCouplingProperties: Reading coupling properties" << endl;
  
  couplingPropertiesPtr = new IOdictionary(IOobject("couplingProperties",runTime.constant(),
                                                    fluidMesh,IOobject::MUST_READ,IOobject::NO_WRITE));
  IOdictionary &couplingProperties(*couplingPropertiesPtr);
  
  solidPatchName = word(couplingProperties.lookup("solidPatch"));
  solidPatchID = structMesh.boundaryMesh().findPatchID(solidPatchName);
  
  if (solidPatchID < 0)
    {
      FatalErrorIn(args.executable())
        << "Problem with finding solid patch"
        << abort(FatalError);
    }
  
  solidZoneName = word(couplingProperties.lookup("solidZone"));
  
  
  solidZoneID =
    structMesh.faceZones().findZoneID(solidZoneName);
  
  if (solidZoneID < 0)
    {
      FatalErrorIn(args.executable())
        << "Problem with finding solid zone"
        << abort(FatalError);
    }
  
  fluidPatchName = word(couplingProperties.lookup("fluidPatch"));
  fluidPatchID = fluidMesh.boundaryMesh().findPatchID(fluidPatchName);
  
  
  if (fluidPatchID < 0)
    {
      FatalErrorIn(args.executable())
        << "Problem with finding fluid patch"
        << abort(FatalError);
    }
  
  fluidZoneName = word(couplingProperties.lookup("fluidZone"));
  fluidZoneID = fluidMesh.faceZones().findZoneID(fluidZoneName);
  if (fluidZoneID < 0)
    {
      FatalErrorIn(args.executable())
        << "Problem with finding fluid zone"
        << abort(FatalError);
    }
  
  feMotionSolver = fluidMesh.objectRegistry::foundObject<tetPointVectorField>("motionU");
  fvMotionSolver = fluidMesh.objectRegistry::foundObject<pointVectorField>("pointMotionU");
  
  // Info << OutStream.str() << endl;
  // OutStream.clear();
  // OutStream.str("");
  
  
  // Grab solid patch field
  Info << "FsiFoam:ReadCouplingProperties:Solid patch ID: " << solidPatchID << endl;
  volVectorField &DURef(*DUPtr);
  if
    (
     DURef.boundaryField()[solidPatchID].type()
     != solidTractionFvPatchVectorField::typeName
     //!= tractionDisplacementIncrementFvPatchVectorField::typeName
     )
    {
      FatalErrorIn(args.executable())
        << "Boundary condition on " << DURef.name()
        <<  " is "
        << DURef.boundaryField()[solidPatchID].type()
        << "for fluid -solid interface patch, instead "
        << solidTractionFvPatchVectorField::typeName
        //<< tractionDisplacementIncrementFvPatchVectorField::typeName
        << abort(FatalError);
    }
  
  //tractionDisplacementIncrementFvPatchVectorField& tForce =
  //      refCast<tractionDisplacementIncrementFvPatchVectorField>
  //    tForcePtr = new solidTractionFvPatchVectorField(refCast<solidTractionFvPatchVectorField>(DU.boundaryField()[solidPatchID]));
  //    solidTractionFvPatchVectorField& tForce = *tForcePtr;
  
  
  accumulatedFluidInterfaceDisplacementHeaderPtr = new IOobject("accumulatedFluidInterfaceDisplacement",
                                                                runTime.timeName(),
                                                                fluidMesh,
                                                                IOobject::MUST_READ);
  IOobject &accumulatedFluidInterfaceDisplacementHeader(*accumulatedFluidInterfaceDisplacementHeaderPtr);
  
  
  
  if(accumulatedFluidInterfaceDisplacementHeader.headerOk())
    {
      Pout << "Reading accumulated fluid interface displacement" << endl;
      
      accumulatedFluidInterfaceDisplacementPtr =
        new vectorIOField
        (
         IOobject
         (
          "accumulatedFluidInterfaceDisplacement",
          runTime.timeName(),
          fluidMesh,
          IOobject::MUST_READ,
          IOobject::AUTO_WRITE
          )
         );
    }
  else
    {
      accumulatedFluidInterfaceDisplacementPtr =
        new vectorIOField
        (
         IOobject
         (
          "accumulatedFluidInterfaceDisplacement",
          runTime.timeName(),
          fluidMesh,
          IOobject::NO_READ,
          IOobject::AUTO_WRITE
          ),
         vectorField
         
         (
          fluidMesh.boundaryMesh()[fluidPatchID].nPoints(),
          vector::zero
          )
            );
    }
  
  
  // sumLocalContErr = 0;
  // globalContErr = 0;
  // cumulativeContErr = 0;
  
  
  
  return(0);
};

int fsifoam_module::CreateInterZoneInterpolators()
{
  
  dynamicFvMesh &fluidMesh = meshPtr();
  fvMesh &structMesh(*stressMeshPtr);
  
  if(!interpolatorFluidSolidPtr || !interpolatorSolidFluidPtr)
    {
      deleteDemandDrivenData(interpolatorFluidSolidPtr);
      deleteDemandDrivenData(interpolatorSolidFluidPtr);
        
      Info << "FsiFoam:CreateInterZoneInterpolators: Create fluid-to-solid and solid-to-fluid interpolators" << endl;
        
      interpolatorFluidSolidPtr = new zoneToZoneInterpolation
        (
         fluidMesh.faceZones()[fluidZoneID](),
         structMesh.faceZones()[solidZoneID](),
         intersection::VISIBLE
         );

      interpolatorSolidFluidPtr = new zoneToZoneInterpolation
        (
         structMesh.faceZones()[solidZoneID](),
         fluidMesh.faceZones()[fluidZoneID](),
         intersection::VISIBLE
         );
        
      Info << "FsiFoam:CreateInterZoneInterpolators: Check fluid-to-solid and solid-to-fluid interpolators" << endl;
        
      {
        vectorField fluidPatchFaceCentres =
          vectorField(fluidMesh.boundaryMesh()[fluidPatchID].faceCentres());
          
        vectorField fluidZoneFaceCentres
          (
           fluidMesh.faceZones()[fluidZoneID].size(),
           vector::zero
           );
          
          
        const label fluidPatchStart =
          fluidMesh.boundaryMesh()[fluidPatchID].start();
          
        forAll (fluidPatchFaceCentres, i)
          {
            fluidZoneFaceCentres
              [
               fluidMesh.faceZones()[fluidZoneID].whichFace(fluidPatchStart + i)
               ] =
              fluidPatchFaceCentres[i];
          }
          
        // Parallel data exchange: collect faceCentres field on all processors
        reduce(fluidZoneFaceCentres, sumOp<vectorField>());

        vectorField solidZoneFaceCentres =
          interpolatorFluidSolidPtr->faceInterpolate
          (
           fluidZoneFaceCentres
           );
          
        vectorField solidPatchFaceCentres
          (
           structMesh.boundaryMesh()[solidPatchID].size(),
           vector::zero
           );
          
        const label solidPatchStart =
          structMesh.boundaryMesh()[solidPatchID].start();
          
        forAll(solidPatchFaceCentres, i)
          {
            solidPatchFaceCentres[i] =
              solidZoneFaceCentres
              [
               structMesh.faceZones()[solidZoneID]
               .whichFace(solidPatchStart + i)
               ];
          }
          
        scalar maxDist = gMax
          (
           mag
           (
            solidPatchFaceCentres
            - structMesh.boundaryMesh()[solidPatchID].faceCentres()
            )
           );
          
        Info << "FsiFoam:CreateInterZoneInterpolators: Fluid-to-solid face interpolation error: " << maxDist
             << endl;
      }
        
        
        
      {
        vectorField solidPatchFaceCentres =
          vectorField(structMesh.boundaryMesh()[solidPatchID].faceCentres());
          
        vectorField solidZoneFaceCentres
          (
           structMesh.faceZones()[solidZoneID].size(),
           vector::zero
           );
          
        const label solidPatchStart =
          structMesh.boundaryMesh()[solidPatchID].start();
          
        forAll (solidPatchFaceCentres, i)
          {
            solidZoneFaceCentres
              [
               structMesh.faceZones()[solidZoneID]
               .whichFace(solidPatchStart + i)
               ] =
              solidPatchFaceCentres[i];
          }
          
        // Parallel data exchange: collect faceCentres field on all processors
        reduce(solidZoneFaceCentres, sumOp<vectorField>());
          
        vectorField fluidZoneFaceCentres =
          interpolatorSolidFluidPtr->faceInterpolate
          (
           solidZoneFaceCentres
           );
          
        vectorField fluidPatchFaceCentres
          (
           fluidMesh.boundaryMesh()[fluidPatchID].size(),
           vector::zero
           );
          
        const label fluidPatchStart =
          fluidMesh.boundaryMesh()[fluidPatchID].start();
          
        forAll(fluidPatchFaceCentres, i)
          {
            fluidPatchFaceCentres[i] =
              fluidZoneFaceCentres
              [
               fluidMesh.faceZones()[fluidZoneID]
               .whichFace(fluidPatchStart + i)
               ];
          }
          
        scalar maxDist = gMax
          (
           mag
           (
            fluidPatchFaceCentres
            - fluidMesh.boundaryMesh()[fluidPatchID].faceCentres()
            )
           );
          
        Info << "FsiFoam:CreateInterZoneInterpolators: Solid-to-fluid face interpolation error: " << maxDist
             << endl;
      }
        
    }
    
  return(0);
};

int fsifoam_module::FindGlobalFaceZones()
{
    
  fvMesh &structMesh(*stressMeshPtr);
  Foam::argList &args = *argsPtr;

  {
    SLList<label> globalFaceZonesSet;
      
    const faceZoneMesh& faceZones = structMesh.faceZones();
      
    forAll(faceZones, zoneI)
      {
        const faceZone& curFaceZone = faceZones[zoneI];
          
        forAll(curFaceZone, faceI)
          {
            // if unused face exist
            if (curFaceZone[faceI] >= structMesh.nFaces())
              {
                globalFaceZonesSet.insert(zoneI);
                break;
              }
          }
      }
    
    globalFaceZones = labelList(globalFaceZonesSet);
  }
  globalToLocalFaceZonePointMap.resize(globalFaceZones.size());

  forAll(globalFaceZones, zoneI)
    {
      label curZoneID = globalFaceZones[zoneI];
      labelList curMap(structMesh.faceZones()[curZoneID]().nPoints(), -1);
      vectorField fzGlobalPoints =
        structMesh.faceZones()[curZoneID]().localPoints();

      //- set all slave points to zero because only the master order is used
      if(!Pstream::master())
        {
          fzGlobalPoints *= 0.0;
        }

      //- pass points to all procs
      reduce(fzGlobalPoints, sumOp<vectorField>());

      //- now every proc has the master's list of FZ points
      //- every proc must now find the mapping from their local FZ points to
      //- the global FZ points
      const vectorField& fzLocalPoints =
        structMesh.faceZones()[curZoneID]().localPoints();
      const edgeList& fzLocalEdges =
        structMesh.faceZones()[curZoneID]().edges();
      const labelListList& fzPointEdges =
        structMesh.faceZones()[curZoneID]().pointEdges();
      scalarField minEdgeLength(fzLocalPoints.size(), GREAT);
    
      forAll(minEdgeLength, pI)
        {
          const labelList& curPointEdges = fzPointEdges[pI];
          forAll(curPointEdges, eI)
            {
              scalar Le = fzLocalEdges[curPointEdges[eI]].mag(fzLocalPoints);
              if (Le < minEdgeLength[pI])
                {
                  minEdgeLength[pI] = Le;
                }
            }
        }
    
      forAll(fzGlobalPoints, globalPointI)
        {
          //         scalar minDist = GREAT;
        
          forAll(fzLocalPoints, procPointI)
            {
              scalar curDist =
                mag
                (
                 fzLocalPoints[procPointI]
                 - fzGlobalPoints[globalPointI]
                 );
            
              //             if (curDist < minDist)
              //             {
              //                 minDist = curDist;
              //             }
            
              if (curDist < 1e-4*minEdgeLength[procPointI])
                {
                  curMap[globalPointI] = procPointI;
                  break;
                }
            }
        
          //         if (curMap[globalPointI] == -1)
          //         {
          //             Pout << "minDist: " << minDist << endl;
          //         }
        }
    
      forAll(curMap, globalPointI)
        {
          if (curMap[globalPointI] == -1)
            {
              FatalErrorIn(args.executable())
                << "local to global face zone point map is not correct"
                << abort(FatalError);
            }
        }
    
      globalToLocalFaceZonePointMap[zoneI] = curMap;
    }
  return(0);
};
int fsifoam_module::ReadPISOControls()
{
  dynamicFvMesh &fluidMesh(meshPtr());
  dictionary &piso(fluidMesh.solutionDict().subDict("PISO"));
  
  nCorr = readInt(piso.lookup("nCorrectors"));
  nNonOrthCorr = piso.lookupOrDefault<int>("nNonOrthogonalCorrectors", 0);
  momentumPredictor = piso.lookupOrDefault<Switch>("momentumPredictor", true);
  transonic = piso.lookupOrDefault<Switch>("transonic", false);
  nOuterCorr = piso.lookupOrDefault<int>("nOuterCorrectors", 1);
  
  return(0);
};
int fsifoam_module::ReadFSIControls()
{
  IOdictionary &couplingProperties(*couplingPropertiesPtr);
  // dynamicFvMesh &fluidMesh(meshPtr());
  // Foam::Time &runTime(*runTimePtr);
  
  if (couplingProperties.found("couplingScheme"))
    {
      couplingScheme = word(couplingProperties.lookup("couplingScheme"));
      if
        (
         (couplingScheme == "IQN-ILS")
         || (couplingScheme == "Aitken")
         || (couplingScheme == "FixedRelaxation")
         || (couplingScheme == "NonIterative")
         )
        {
          Info<< "FsiFoam:ReadFSIControl: Selecting coupling scheme " << couplingScheme << endl;
        }
      else
        {
          FatalErrorIn
            (
             "readFsiProperties"
             )   << "couplingScheme: " << couplingScheme
                 << " is not a valid choice. "
                 << "Options are: IQN-ILS, Aitken, FixedRelaxation"
                 << abort(FatalError);
        }
    }

  interfaceDeformationLimit = 
    scalar(readScalar(couplingProperties.lookup("interfaceDeformationLimit")));


  // if(dynamicMeshDictPtr)
  //   delete dynamicMeshDictPtr;
  // dynamicMeshDictPtr = new IOdictionary(IOobject
  //                                       (
  //                                        "dynamicMeshDict",
  //                                        runTime.constant(),
  //                                        fluidMesh,
  //                                        IOobject::MUST_READ,
  //                                        IOobject::NO_WRITE,
  //                                        false
  //                                        ));



  fsiRelaxationFactorMin = scalar(readScalar(couplingProperties.lookup("fsiRelaxationFactor")));
  fsiRelaxationFactor = fsiRelaxationFactorMin;
  outerCorrTolerance = scalar(readScalar(couplingProperties.lookup("outerCorrTolerance")));
  Info << "FsiFoam:ReadFSIControl: outerCorrTolerance = " << outerCorrTolerance << endl;
  fsi = Switch(couplingProperties.lookup("fsi"));
  if(fsi)
    Info << "FsiFoam:ReadFSIControl: FSI IS ENABLED." << endl;
  else
    Info << "FsiFoam:ReadFSIControl: FSI IS DISABLED." << endl;
  return(0);
};

int fsifoam_module::Step(){
  Foam::argList &args(this->ArgList());
  Foam::Time &runTime(this->RunTime());
  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  //  IOdictionary &transportProperties(this->TransportProperties());
  dimensionedScalar &nu(this->nu());
  dimensionedScalar &rhoFluid(this->rhoFluid());
  volScalarField &p(this->p());
  volVectorField &U(this->U());
  surfaceScalarField &phi(this->phi());
  // label pRefCell = 0;
  // scalar pRefValue = 0.0;
  // setRefCell(p, fluidsMesh.solutionDict().subDict("PISO"), pRefCell, pRefValue);
    
  fvMesh &structuresMesh(this->StructuresMesh());
    
    
  volVectorField &DU(this->DU());
  volTensorField &gradDU(this->gradDU());
  volVectorField &Usolid(this->Usolid());
  volVectorField &DV(this->DV());
  volVectorField &Vs(this->Vs());
  volSymmTensorField &sigma(this->sigma());
  volSymmTensorField &DSigma(this->DSigma());
  //  constitutiveModel &Rheology(){return(*rheologyPtr);};
  volScalarField &rho(this->rhoSolid());
  volScalarField &mu(this->mu());
  volScalarField &lambda(this->lambda());
  volTensorField &F(this->F());
  volTensorField &DF(this->DF());
  volScalarField &J(this->J());
  word &solidDdtScheme(this->SolidDdtScheme());
  //  pointMesh &pStressMesh(){return(*pStressMeshPtr);};
  //  pointVectorField &PointDU(){return(*pointDUPtr);};
    
    
  //  IOdictionary &couplingProperties(this->CouplingProperties());
  label solidPatchID(this->SolidPatchID());
  //  word  solidPatchName(this->SolidPatchName());
  label fluidPatchID(this->FluidPatchID());
  //  word fluidPatchName(this->FluidPatchName());
  label fluidZoneID(this->FluidZoneID());
  label solidZoneID(this->SolidZoneID());
    
  bool feMotionSolver(this->FEMotion());
  bool fvMotionSolver(this->FVMotion());
  vectorField &accumulatedFluidInterfaceDisplacement(this->AccumulatedFluidInterfaceDisplacements());
  // Masoud:
  Info << "accumulatedFluidInterfaceDisplacement = " << endl;
  Info << accumulatedFluidInterfaceDisplacement << endl;
  // Masoud: End
  solidTractionFvPatchVectorField &tForce(this->tForce());
    
  zoneToZoneInterpolation &interpolatorFluidSolid(this->interpFluidSolid());
  zoneToZoneInterpolation &interpolatorSolidFluid(this->interpSolidFluid());
    
  scalar &sumLocalContErr(this->LocalContErr());
  scalar &globalContErr(this->GlobalContErr());
  scalar &cumulativeContErr(this->CumulativeContErr());
    
  labelList &globalFaceZones(this->GlobalFaceZones());
  labelListList &globalToLocalFaceZonePointMap(this->GlobalToLocalFaceZonePointMap());
  runTime++;
  if(!runTime.end()){
    //    for (runTime++; !runTime.end(); runTime++)
    //      {
    // Info << "Time = " << runTime.timeName() << nl << endl;
      
    //#       include "readPISOControls.H"
    //#       include "readFsiControls.H"
    //#       include "createStressPointMesh.H"
      
    this->ReadPISOControls();
    this->ReadFSIControls();
    word couplingScheme(this->CouplingScheme());
    //        label &outerCorr(this->OuterCorr());
    scalar &fsiRelaxationFactor(this->FSIRelaxationFactor());
    //        scalar &fsiRelaxationFactorMin(this->FSIRelaxationFactorMin());
    Switch fsi(this->FSIEnabled());
    int &nCorrPISO(this->NCorrPISO());
    int &nNonOrthCorr(this->NNonOrthCorr());
    scalar &interfaceDeformationLimit(this->InterfaceDeformationLimit());
    scalar &outerCorrTolerance(this->OuterCorrTolerance());
    int &nOuterCorr(this->NOuterCorr());
      
    //        this->CreateStructuresPointMesh();


    // Create point stress mesh for interpolation
    pointMesh StructuresPointMesh(structuresMesh);
        
    pointPatchInterpolation patchPointInterpolator(structuresMesh);
        
        
    // Create point displacement field (check fixedValue patches)
        
    wordList types
      (
       StructuresPointMesh.boundary().size(),
       calculatedFvPatchVectorField::typeName
       );
        
    // wordList types = DU.boundaryField().types();
        
    forAll(DU.boundaryField().types(), patchI)
      {
        if
          (
           DU.boundaryField().types()[patchI]
           == fixedValueFvPatchVectorField::typeName
           )
          {
            types[patchI] = fixedValueFvPatchVectorField::typeName;
          }
      }
        
    pointVectorField pointDU
      (
       IOobject
       (
        "pointDU",
        runTime.timeName(),
        structuresMesh
        ),
       StructuresPointMesh,
       dimensionedVector("zero", dimLength, vector::zero),
       types
       );
    //#       include "createInterfaceFields.H"
    vectorField fluidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fluidPatchPointsDisplOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField solidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidual
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidualOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField solidPatchTraction
      (
       structuresMesh.boundary()[solidPatchID].size(),
       vector::zero
       );
        
    scalarField solidPatchPressure
      (
       structuresMesh.boundary()[solidPatchID].size(),
       0.0
       );
        
    scalar initialFsiResidualNorm = 0;
    scalar fsiResidualNorm = 0;        
    label outerCorr=0;

    do
      {
        outerCorr++;

        //#           include "setInterfaceDisplacement.H"
        Info << "\nTime = " << runTime.timeName()
             << ", iteration: " << outerCorr << endl;
              
        if (outerCorr < 3 || couplingScheme == "FixedRelaxation")
          {
            Info << "Current fsi under-relaxation factor: "
                 << fsiRelaxationFactor << endl;
                  
            fluidPatchPointsDisplOld = fluidPatchPointsDispl;
                  
            fluidPatchPointsDispl += fsiRelaxationFactor*fsiResidual;
          }
        else
          {
            if (couplingScheme == "Aitken")
              {
                fsiRelaxationFactor =
                  -fsiRelaxationFactor
                  *(
                    gSum
                    (
                     fsiResidualOld
                     &(fsiResidual - fsiResidualOld)
                     )
                    /(
                      gSum
                      (
                       (fsiResidual - fsiResidualOld)
                       &(fsiResidual - fsiResidualOld)
                       )
                      )
                    );
                      
                fsiRelaxationFactor = mag(fsiRelaxationFactor);
                      
                Info << "Current fsi under-relaxation factor (Aitken): "
                     << fsiRelaxationFactor << endl;
                      
                fluidPatchPointsDisplOld = fluidPatchPointsDispl;
                      
                fluidPatchPointsDispl +=
                  fsiRelaxationFactor*fsiResidual;
              }
            //     else if (couplingScheme == "IQN-ILS")
            //     {
                  
            //     }
          }
              
        //#           include "moveFluidMesh.H"
        {
          // Move fluid mesh
          const vectorField& n =
            fluidsMesh.boundaryMesh()[fluidPatchID].pointNormals();
                  
          primitivePatchInterpolation patchInterpolator
            (
             fluidsMesh.boundaryMesh()[fluidPatchID]
             );
                  
          scalarField pointDeltaCoeffs =
            patchInterpolator.faceToPointInterpolate
            (
             fluidsMesh.boundary()[fluidPatchID].deltaCoeffs()
             );
                  
          scalar delta =
            gMax
            (
             mag
             (
              n
              & (
                 accumulatedFluidInterfaceDisplacement
                 + fluidPatchPointsDispl
                 - fluidPatchPointsDisplOld
                 )
              )
             *pointDeltaCoeffs
             );
                  
          Info << "Maximal accumulated displacement of interface points: "
               << delta << endl;
                  
          if(delta < interfaceDeformationLimit)
            {
              // Move only interface points
              // Masoud
              Info << "Moving only interface points..." << endl;
              // Masoud: End
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (fluidPatchPointsDispl, pointI)
                {
                  //Info << "1. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;
                  newPoints[meshPoints[pointI]] +=
                    fluidPatchPointsDispl[pointI]
                    - fluidPatchPointsDisplOld[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              // Accumulate interface points displacement
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl
                - fluidPatchPointsDisplOld;
            }
          else
            {
              // Move whole fluid mesh
              // Masoud
              Info << "Moving the whole mesh .... " << endl;
              // Masoud: End
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (accumulatedFluidInterfaceDisplacement, pointI)
                {
                  //Info << "2. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;
                  newPoints[meshPoints[pointI]] -=
                    accumulatedFluidInterfaceDisplacement[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl
                - fluidPatchPointsDisplOld;
                      
                      
              if (feMotionSolver)
                {
                  tetPointVectorField& motionU =
                    const_cast<tetPointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<tetPointVectorField>
                     (
                      "motionU"
                      )
                     );
                          
                  fixedValueTetPolyPatchVectorField& motionUFluidPatch =
                    refCast<fixedValueTetPolyPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                          
                  tetPolyPatchInterpolation tppi
                    (
                     refCast<const faceTetPolyPatch>(motionUFluidPatch.patch())
                     );
                          
                  motionUFluidPatch ==
                    tppi.pointToPointInterpolate
                    (
                     accumulatedFluidInterfaceDisplacement
                     /runTime.deltaT().value()
                     );
                }
              else if (fvMotionSolver)
                {
                  pointVectorField& motionU =
                    const_cast<pointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<pointVectorField>
                     (
                      "pointMotionU"
                      )
                     );
                
                  fixedValuePointPatchVectorField& motionUFluidPatch =
                    refCast<fixedValuePointPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                
                  motionUFluidPatch ==
                    accumulatedFluidInterfaceDisplacement
                    /runTime.deltaT().value();
                }
              else
                {
                  FatalErrorIn(args.executable())
                    << "Problem with mesh motion solver selection"
                    << abort(FatalError);
                }
                      
              fluidsMesh.update();
                      
              accumulatedFluidInterfaceDisplacement = vector::zero;
            }
        }
                
        //            Info << "DU after move fluid: " << DU << endl;
                
        //#           include "solveFluid.H"
        if(fluidsMesh.moving())
          {
            // Make the fluxes relative
            phi -= fvc::meshPhi(U);
          }
                  

        //#       include "CourantNo.H"
        scalar CoNum = 0.0;
        scalar meanCoNum = 0.0;
        scalar velMag = 0.0;
                    
        if (fluidsMesh.nInternalFaces())
          {
            surfaceScalarField magPhi = mag(phi);
                        
            surfaceScalarField SfUfbyDelta =
              fluidsMesh.surfaceInterpolation::deltaCoeffs()*magPhi;
                        
            const scalar deltaT = runTime.deltaT().value();
                        
            CoNum = max(SfUfbyDelta/fluidsMesh.magSf()).value()*deltaT;
                        
            meanCoNum = (sum(SfUfbyDelta)/sum(fluidsMesh.magSf())).value()*deltaT;
                        
            velMag = max(magPhi/fluidsMesh.magSf()).value();
          }
                    
        Info<< "Courant Number mean: " << meanCoNum
            << " max: " << CoNum
            << " velocity magnitude: " << velMag
            << endl;
                    
        fvVectorMatrix UEqn
          (
           fvm::ddt(U)
           + fvm::div(phi, U)
           - fvm::laplacian(nu, U)
           );
                  
        solve(UEqn == -fvc::grad(p));
                  
        // --- PISO loop
        volScalarField rUA = 1.0/UEqn.A();
                  
        for (int corr=0; corr<nCorrPISO; corr++)
          {
            U = rUA*UEqn.H();
            phi = (fvc::interpolate(U) & fluidsMesh.Sf());
                      
            adjustPhi(phi, U, p);
                      
            for (int nonOrth=0; nonOrth<=nNonOrthCorr; nonOrth++)
              {
                fvScalarMatrix pEqn
                  (
                   fvm::laplacian(rUA, p)
                   == fvc::div(phi)
                   );
                          
                pEqn.setReference(pRefCell, pRefValue);
                pEqn.solve();
                          
                if (nonOrth == nNonOrthCorr)
                  {
                    phi -= pEqn.flux();
                  }
              }
                      
            //#           include "continuityErrs.H"
            {
              volScalarField contErr = fvc::div(phi);
                        
              sumLocalContErr = runTime.deltaT().value()*
                mag(contErr)().weightedAverage(fluidsMesh.V()).value();
                        
              globalContErr = runTime.deltaT().value()*
                contErr.weightedAverage(fluidsMesh.V()).value();
                        
              cumulativeContErr += globalContErr;
                        
              Info<< "time step continuity errors : sum local = " << sumLocalContErr
                  << ", global = " << globalContErr
                  << ", cumulative = " << cumulativeContErr
                  << endl;
            }
                      
            U -= rUA*fvc::grad(p);
            U.correctBoundaryConditions();
          }
                  
        //            Info << "DU after solve fluid: " << DU << endl;
                  
        //#           include "setInterfaceForce.H"
        {
          Info << "Setting traction on solid patch" << endl;
                      
          //     vectorField fluidPatchTraction =
          //        -rhoFluid.value()*nu.value()
          //        *U.boundaryField()[fluidPatchID].snGrad()
          //       + rhoFluid.value()*p.boundaryField()[fluidPatchID]
          //        *fluidsMesh.boundary()[fluidPatchID].nf();
                      
          vectorField fluidPatchTraction =
            -rhoFluid.value()*nu.value()
            *U.boundaryField()[fluidPatchID].snGrad();
                      
          //    Info << "Fluid patch traction: " << fluidPatchTraction << endl;
                      
          scalarField fluidPatchPressure =
            rhoFluid.value()*p.boundaryField()[fluidPatchID];
                      
          //    Info << "Fluid patch pressure: " << fluidPatchPressure << endl;
                      
          vectorField fluidZoneTraction
            (
             fluidsMesh.faceZones()[fluidZoneID].size(),
             vector::zero
             );
                      
          //    Info << "Fluid zone traction: " << fluidZoneTraction << endl;
                      
          const label fluidPatchStart =
            fluidsMesh.boundaryMesh()[fluidPatchID].start();
                      
          forAll(fluidPatchTraction, i)
            {
              fluidZoneTraction
                [
                 fluidsMesh.faceZones()[fluidZoneID].whichFace(fluidPatchStart + i)
                 ] =
                fluidPatchTraction[i];
            }
                      
          // Parallel data exchange: collect pressure field on all processors
          reduce(fluidZoneTraction, sumOp<vectorField>());
                      
                      
          scalarField fluidZonePressure
            (
             fluidsMesh.faceZones()[fluidZoneID].size(),
             0.0
             );
                      
          forAll(fluidPatchPressure, i)
            {
              fluidZonePressure
                [
                 fluidsMesh.faceZones()[fluidZoneID].whichFace(fluidPatchStart + i)
                 ] =
                fluidPatchPressure[i];
            }
                      
          // Parallel data exchange: collect pressure field on all processors
          reduce(fluidZonePressure, sumOp<scalarField>());
                      
          vectorField solidZoneTraction =
            interpolatorFluidSolid.faceInterpolate
            (
             fluidZoneTraction
             );
                      
          //    Info << "Solid Zone Traction: " << solidZoneTraction << endl;
                      
          scalarField solidZonePressure =
            interpolatorFluidSolid.faceInterpolate
            (
             fluidZonePressure
             );
                      
          //    Info << "Solid Zone Pressure: " << solidZonePressure << endl;
                      
          const label solidPatchStart =
            structuresMesh.boundaryMesh()[solidPatchID].start();
                      
          forAll(solidPatchTraction, i)
            {
              solidPatchTraction[i] =
                solidZoneTraction
                [
                 structuresMesh.faceZones()[solidZoneID]
                 .whichFace(solidPatchStart + i)
                 ];
            }
                      
          forAll(solidPatchPressure, i)
            {
              solidPatchPressure[i] =
                solidZonePressure
                [
                 structuresMesh.faceZones()[solidZoneID]
                 .whichFace(solidPatchStart + i)
                 ];
            }
                      
          if (fsi)
            {
              tForce.traction() = solidPatchTraction;
              tForce.pressure() = solidPatchPressure;
            }
                      
          vector totalTractionForce =
            gSum
            (
             solidPatchTraction
             *structuresMesh.magSf().boundaryField()[solidPatchID]
             );
                      
          Info << "Total traction force = "
               << totalTractionForce << endl;
        }
                    
        //            Info << "DU after setting interface force: " << DU << endl;
                    
        //#           include "solveSolid.H"
        if (solidDdtScheme == fv::EulerDdtScheme<vector>::typeName)
          {
            Info << "Solve Solid: Euler" << endl;
            //#   include "solveSolidEuler.H"
            {
              const dictionary& stressControl =
                structuresMesh.solutionDict().subDict("solidMechanics");
                              
              int nCorrStruct(readInt(stressControl.lookup("nCorrectors")));
              scalar convergenceTolerance(readScalar(stressControl.lookup("DU")));
              //#       include "readSolidMechanicsControls.H"
                                
              int iCorr = 0;
              lduMatrix::solverPerformance solverPerf;
              scalar initialResidual = 0;
                              
              lduMatrix::debug = 0;
                              
              //#       include "EulerCoeffs.H"
              dimensionedScalar Cn("Cn", dimless/dimTime, 1.0/runTime.deltaT().value());
              dimensionedScalar Co("Co", dimless/dimTime, 1.0/runTime.deltaT().value());
                              
              do
                {
                  DU.storePrevIter();
                                  
                  fvVectorMatrix DUEqn
                    (
                     Cn*rho*fvm::ddt(DU)
                     - Co*rho*DV.oldTime()
                     ==
                     fvm::laplacian(2*mu + lambda, DU, "laplacian(DDU,DU)")
                     - fvc::laplacian(mu + lambda, DU, "laplacian(DDU,DU)")
                     + fvc::div
                     (
                      mu*gradDU.T()
                      + lambda*(I*tr(gradDU))
                      + mu*(gradDU&gradDU.T())
                      + 0.5*lambda*(I*tr(gradDU & gradDU.T()))
                      + (sigma & DF.T())
                      + (DSigma & DF.T()),
                      "div(sigma)"
                      )
                     );
                                  
                  solverPerf = DUEqn.solve();
                                  
                  DU.relax();
                                  
                  if(iCorr == 0)
                    {
                      initialResidual = solverPerf.initialResidual();
                    }
                                  
                  gradDU = fvc::grad(DU);
                                  
                  DF = gradDU.T();
                                  
                  //#           include "calculateDSigma.H"
                  {
                    volSymmTensorField Depsilon = symm(gradDU)
                      + 0.5*symm(gradDU & gradDU.T());
                                    
                    DSigma = 2*mu*Depsilon + I*(lambda*tr(Depsilon));
                  }
                }
              while
                (
                 solverPerf.initialResidual() > convergenceTolerance
                 && ++iCorr < nCorrStruct
                 );
                              
              Info << "Solving for " << DU.name()
                   << ", Initial residual = " << initialResidual
                   << ", Final residual = " << solverPerf.initialResidual()
                   << ", No outer iterations " << iCorr << endl;
                              
              DV = fvc::ddt(DU);
                              
              lduMatrix::debug = 1;
            }
          }
        else if (solidDdtScheme == fv::backwardDdtScheme<vector>::typeName)
          {
            Info << "Solve Solid: Backward" << endl;
            //#   include "solveSolidBackward.H"
                            
            {
              //#       include "readSolidMechanicsControls.H"
              const dictionary& stressControl =
                structuresMesh.solutionDict().subDict("solidMechanics");
                            
              int nCorrStruct(readInt(stressControl.lookup("nCorrectors")));
              scalar convergenceTolerance(readScalar(stressControl.lookup("DU")));
                            
              int iCorr = 0;
              lduMatrix::solverPerformance solverPerf;
              scalar initialResidual = 0;
                            
              lduMatrix::debug = 0;

              //#       include "backwardCoeffs.H"
              dimensionedScalar Cn("Cn", dimless/dimTime, 0.0);
              dimensionedScalar Co("Co", dimless/dimTime, 0.0);
              dimensionedScalar Coo("Coo", dimless/dimTime, 0.0);
                              
              scalar deltaT = runTime.deltaT().value();
              scalar deltaT0 = runTime.deltaT0().value();
                              
              Cn.value() = 1 + deltaT/(deltaT + deltaT0);
              Coo.value() = deltaT*deltaT/(deltaT0*(deltaT + deltaT0));
              Co.value() = Cn.value() + Coo.value();
                              
              if(runTime.timeIndex() == 1)
                {
                  Cn.value() = 1.0;
                  Co.value() = 1.0;
                  Coo.value() = 0.0;
                }
                              
              Cn.value() /= deltaT;
              Co.value() /= deltaT;
              Coo.value() /= deltaT;
                              
              // Info << "ncorr = " << nCorr << endl;
              // Info << "convergence Tol: " << convergenceTolerance << endl;
              // Info << "DU:" << DU << endl;
              // Info << Cn << "," << Co << "," << Coo << endl;
              // Info << "RHO: " << rho << endl;
              // Info << "MU: " << mu << endl;
              // Info << "LAMBDA: " << lambda << endl;
              // Info << "gradDU:" << gradDU << endl;
              // Info << "gradDU.T(): " << gradDU.T() << endl;
              // Info << "DF: " << DF << endl;
              // Info << "DF.T(): " << DF.T() << endl;
              // Info << "SIGMA: " << sigma << endl;
              // Info << "DSigma: " << DSigma << endl;
              // Info << "DV: " << DV << endl;
              // Info << "DV.oldTime(): " << DV.oldTime() << endl;
              // Info << "DV.oldTime().oldTime(): " << DV.oldTime().oldTime() << endl;
                              
              do
                {
                  DU.storePrevIter();
                                  
                  fvVectorMatrix DUEqn
                    (
                     Cn*rho*fvm::ddt(DU)
                     - Co*rho*DV.oldTime()
                     + Coo*rho*DV.oldTime().oldTime()
                     ==
                     fvm::laplacian(2*mu + lambda, DU, "laplacian(DDU,DU)")
                     - fvc::laplacian(mu + lambda, DU, "laplacian(DDU,DU)")
                     + fvc::div
                     (
                      mu*gradDU.T()
                      + lambda*(I*tr(gradDU))
                      + mu*(gradDU&gradDU.T())
                      + 0.5*lambda*(I*tr(gradDU & gradDU.T()))
                      + (sigma & DF.T())
                      + (DSigma & DF.T()),
                      "div(sigma)"
                      )
                     );
                                  
                  solverPerf = DUEqn.solve();
                                  
                  DU.relax();
                                  
                  if(iCorr == 0)
                    {
                      initialResidual = solverPerf.initialResidual();
                    }
                                  
                  gradDU = fvc::grad(DU);
                                  
                  DF = gradDU.T();
                                  
                  //#           include "calculateDSigma.H"
                  {
                    volSymmTensorField Depsilon = symm(gradDU)
                      + 0.5*symm(gradDU & gradDU.T());
                                    
                    DSigma = 2*mu*Depsilon + I*(lambda*tr(Depsilon));
                  }
                  // Info << "Solving for " << DU.name()
                  //      << ", Initial residual = " << initialResidual
                  //      << ", Final residual = " << solverPerf.initialResidual()
                  //      << ", No outer iterations " << iCorr << endl;
                                  
                }
              while
                (
                 solverPerf.initialResidual() > convergenceTolerance
                 && ++iCorr < nCorrStruct
                 );
                              
              Info << "Solving for " << DU.name()
                   << ", Initial residual = " << initialResidual
                   << ", Final residual = " << solverPerf.initialResidual()
                   << ", No outer iterations " << iCorr << endl;
                              
              DV = fvc::ddt(DU);
                              
              lduMatrix::debug = 1;
            }
          }
        else
          {
            FatalErrorIn(args.executable())
              << "Wrong temporal (ddt) scheme for solid solver. "
              << "Possible schemes are: "
              << fv::EulerDdtScheme<vector>::typeName << " and "
              << fv::backwardDdtScheme<vector>::typeName
              << abort(FatalError);
          }
                      
        //#           include "calcFsiResidual.H"
        {
          const vectorField& solidPatchDisplacement =
            DU.boundaryField()[solidPatchID];
                          
          vectorField solidZoneDisplacement
            (
             structuresMesh.faceZones()[solidZoneID]().size(),
             vector::zero
             );
                          
          const label solidPatchStart =
            structuresMesh.boundaryMesh()[solidPatchID].start();
                          
          forAll(solidPatchDisplacement, i)
            {
              solidZoneDisplacement
                [
                 structuresMesh.faceZones()[solidZoneID]
                 .whichFace(solidPatchStart + i)
                 ] =
                solidPatchDisplacement[i];
            }
                          
          // Parallel data exchange: collect displacement field on all processors
          reduce(solidZoneDisplacement, sumOp<vectorField>());
                          
          vectorField fluidZoneDisplacement =
            interpolatorSolidFluid.faceInterpolate
            (
             solidZoneDisplacement
             );
                          
          vectorField fluidPatchDisplacement
            (
             fluidsMesh.boundary()[fluidPatchID].size(),
             vector::zero
             );
                          
          const label fluidPatchStart =
            fluidsMesh.boundaryMesh()[fluidPatchID].start();
                          
          forAll(fluidPatchDisplacement, i)
            {
              fluidPatchDisplacement[i] =
                fluidZoneDisplacement
                 [
                 fluidsMesh.faceZones()[fluidZoneID].whichFace(fluidPatchStart + i)
                 ];
            }
                          
          primitivePatchInterpolation fluidPatchInterpolator
            (
             fluidsMesh.boundaryMesh()[fluidPatchID]
             );
                          
          solidPatchPointsDispl =
            fluidPatchInterpolator.faceToPointInterpolate
            (
             fluidPatchDisplacement
             );
                          
          fsiResidualOld = fsiResidual;
                          
          fsiResidual = solidPatchPointsDispl - fluidPatchPointsDispl;

          // Masoud
          Info << "solidPatchPointsDispl = " << endl;
          Info << solidPatchPointsDispl << endl;
          Info << "fluidPatchPointsDispl = " << endl;
          Info << fluidPatchPointsDispl << endl;
          // Masoud : End
                          
          //     maxFsiResidual =
          //        gMax
          //         (
          //             mag(fsiResidual)
          //            /(mag(solidPatchPointsDispl) + SMALL)
          //         );
                          
          //     Info << "Maximal fsi residual: " << maxFsiResidual << endl;
                          
          fsiResidualNorm = ::sqrt(gSum(magSqr(fsiResidual)));
                          
          if (outerCorr == 1)
            {
              initialFsiResidualNorm = fsiResidualNorm;
            }
                          
          fsiResidualNorm /= initialFsiResidualNorm + SMALL;
                          
          Info << "Current fsi residual norm: " << fsiResidualNorm << endl;
        }
      }
    while
      (
       (fsiResidualNorm > outerCorrTolerance)
       && (outerCorr < nOuterCorr)
       );
        
    Vs += DV;
        
    //#       include "rotateSolidFields.H"
    {
      Info << "Rotating fields" << endl;
          
      F = I + DF;
          
      //U += DU;
      Usolid += DU;
          
      //epsilon += DEpsilon;
          
      sigma += DSigma;
          
      //volTensorField Finv = inv(F);
          
      J = det(F);
          
      rho = rho/J;
            
      //epsilon = symm(Finv.T() & epsilon & Finv);
          
      sigma  = 1/J * symm(F & sigma & F.T());
    }
    //#       include "moveSolidMeshLeastSquares.H"
    //--------------------------------------------------//
    //- move mesh
    //--------------------------------------------------//
    if(min(J.internalField()) > 0)
      {
        Info << "Moving mesh using least squares interpolation" << endl;
            
        leastSquaresVolPointInterpolation pointInterpolation(structuresMesh);
            
        // Create point mesh
        pointMesh pMesh(structuresMesh);
            
        wordList types
          (
           pMesh.boundary().size(),
           calculatedFvPatchVectorField::typeName
           );
            
        pointVectorField pointDU
          (
           IOobject
           (
            "pointDU",
            runTime.timeName(),
            structuresMesh
            ),
           pMesh,
           dimensionedVector("zero", dimLength, vector::zero),
           types
           );
            
        pointInterpolation.interpolate(DU, pointDU);
            
        const vectorField& pointDUI =
          pointDU.internalField();
            
        //- Move mesh
        vectorField newPoints = structuresMesh.allPoints();
            
        forAll (pointDUI, pointI)
          {
            newPoints[pointI] += pointDUI[pointI];
          }
            
        // Correct symmetryPlane points
            
        forAll(structuresMesh.boundaryMesh(), patchI)
          {
            if (isA<symmetryPolyPatch>(structuresMesh.boundaryMesh()[patchI]))
              {
                const labelList& meshPoints =
                  structuresMesh.boundaryMesh()[patchI].meshPoints();
                    
                vector avgN =
                  gAverage(structuresMesh.boundaryMesh()[patchI].pointNormals());
                    
                vector i(1, 0, 0);
                vector j(0, 1, 0);
                vector k(0, 0, 1);
                    
                if (mag(avgN&i) > 0.95)
                  {
                    forAll(meshPoints, pI)
                      {
                        newPoints[meshPoints[pI]].x() = 0;
                      }
                  }
                else if (mag(avgN&j) > 0.95)
                  {
                    forAll(meshPoints, pI)
                      {
                        newPoints[meshPoints[pI]].y() = 0;
                      }
                  }
                else if (mag(avgN&k) > 0.95)
                  {
                    forAll(meshPoints, pI)
                      {
                        newPoints[meshPoints[pI]].z() = 0;
                      }
                  }
              }
          }
            
        //#   include "calcUnusedNewPoints.H"
        forAll(globalFaceZones, zoneI)
          {
            const label curZoneID = globalFaceZones[zoneI];
                  
            const labelList& curMap =
              globalToLocalFaceZonePointMap[zoneI];
                  
            const labelList& curZoneMeshPoints =
              structuresMesh.faceZones()[curZoneID]().meshPoints();
                  
            vectorField curGlobalZonePointDispl
              (
               curZoneMeshPoints.size(),
               vector::zero
               );
                  
            //-Inter-proc points are shared by multiple procs
            // pointNumProc is the number of procs which a point lies on
            scalarField pointNumProcs(curZoneMeshPoints.size(), 0);
                  
            forAll(curGlobalZonePointDispl, globalPointI)
              {
                label localPoint = curMap[globalPointI];
                      
                if(curZoneMeshPoints[localPoint] < structuresMesh.nPoints())
                  {
                    label procPoint = curZoneMeshPoints[localPoint];
                          
                    curGlobalZonePointDispl[globalPointI] = pointDUI[procPoint];
                          
                    pointNumProcs[globalPointI] = 1;
                  }
              }
                  
            if (Pstream::parRun())
              {
                reduce(curGlobalZonePointDispl, sumOp<vectorField>());
                reduce(pointNumProcs, sumOp<scalarField>());
                      
                //- now average the displacement between all procs
                curGlobalZonePointDispl /= pointNumProcs;
              }
                  
            //- The curZonePointsDisplGlobal now contains the correct face zone
            //  displacement in a global master processor order, now convert them
            //  back into the local proc order
                  
            vectorField curZonePointDispl
              (
               curZoneMeshPoints.size(),
               vector::zero
               );
                  
            forAll(curGlobalZonePointDispl, globalPointI)
              {
                label localPoint = curMap[globalPointI];
                      
                curZonePointDispl[localPoint] =
                  curGlobalZonePointDispl[globalPointI];
              }
                  
            forAll(curZonePointDispl, pointI)
              {
                // unused points
                if (curZoneMeshPoints[pointI] >= structuresMesh.nPoints())
                  {
                    newPoints[curZoneMeshPoints[pointI]] +=
                      curZonePointDispl[pointI];
                  }
              }
          }
              
        twoDPointCorrector twoDCorrector(structuresMesh);
        twoDCorrector.correctPoints(newPoints);
        structuresMesh.movePoints(newPoints);
        structuresMesh.V00();
        structuresMesh.moving(false);
      }
    else
      {
        FatalErrorIn(args.executable())
          << "Negative Jacobian"
          << exit(FatalError);
      }
    //#       include "calculateStress.H"
    if (runTime.outputTime())
      {
        volScalarField sigmaEq
          (
           IOobject
           (
            "sigmaEq",
            runTime.timeName(),
            structuresMesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
            ),
           sqrt((3.0/2.0)*magSqr(dev(sigma)))
           );
              
        Info<< "Max sigmaEq = " << max(sigmaEq).value()
            << endl;
              
        runTime.write();
      }
          
    //#       include "calculateLiftAndDrag.H"
          
    // Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
    //     << "  ClockTime = " << runTime.elapsedClockTime() << " s"
    //     << endl << endl;
  }
  return 0;
};

int fsifoam_module::StepFluidAlone(){

  Foam::argList &args(this->ArgList());
  Foam::Time &runTime(this->RunTime());
  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  //  IOdictionary &transportProperties(this->TransportProperties());
  dimensionedScalar &nu(this->nu());
  dimensionedScalar &rhoFluid(this->rhoFluid());
  volScalarField &p(this->p());
  volVectorField &U(this->U());
  surfaceScalarField &phi(this->phi());
    
    
  // //  IOdictionary &couplingProperties(this->CouplingProperties());

  label fluidPatchID(this->FluidPatchID());
  label fluidZoneID(this->FluidZoneID());
  label solidZoneID(this->SolidZoneID());
    
  bool feMotionSolver(this->FEMotion());
  bool fvMotionSolver(this->FVMotion());
  vectorField &accumulatedFluidInterfaceDisplacement(this->AccumulatedFluidInterfaceDisplacements());
  // Masoud : Checking AccumulatedFluidInterfaceDisplacement
  Info << "Checking AccumulatedFluidInterfaceDisplacementi = " << endl;
  Info << accumulatedFluidInterfaceDisplacement;
  // Masoud: End
  solidTractionFvPatchVectorField &tForce(this->tForce());
    
  zoneToZoneInterpolation &interpolatorFluidSolid(this->interpFluidSolid());
  zoneToZoneInterpolation &interpolatorSolidFluid(this->interpSolidFluid());
    
  scalar &sumLocalContErr(this->LocalContErr());
  scalar &globalContErr(this->GlobalContErr());
  scalar &cumulativeContErr(this->CumulativeContErr());
    
  labelList &globalFaceZones(this->GlobalFaceZones());
  labelListList &globalToLocalFaceZonePointMap(this->GlobalToLocalFaceZonePointMap());
  runTime++;
  if(!runTime.end()){
      
    this->ReadPISOControls();
    this->ReadFSIControls();
    word couplingScheme(this->CouplingScheme());
    //        label &outerCorr(this->OuterCorr());
    scalar &fsiRelaxationFactor(this->FSIRelaxationFactor());
    //        scalar &fsiRelaxationFactorMin(this->FSIRelaxationFactorMin());
    Switch fsi(this->FSIEnabled());
    int &nCorrPISO(this->NCorrPISO());
    int &nNonOrthCorr(this->NNonOrthCorr());
    scalar &interfaceDeformationLimit(this->InterfaceDeformationLimit());
    scalar &outerCorrTolerance(this->OuterCorrTolerance());
    int &nOuterCorr(this->NOuterCorr());
      
    vectorField fluidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fluidPatchPointsDisplOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField solidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidual
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidualOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
        
    scalar initialFsiResidualNorm = 0;
    scalar fsiResidualNorm = 0;        
    label outerCorr=0;

    do
      {
        outerCorr++;
        Info << "outerCorr = " << outerCorr << endl;
        
        //#           include "setInterfaceDisplacement.H"
        Info << "\nTime = " << runTime.timeName()
             << ", iteration: " << outerCorr << endl;
        
        if (outerCorr < 3 || couplingScheme == "FixedRelaxation")
          {
            Info << "Current fsi under-relaxation factor: "
                 << fsiRelaxationFactor << endl;
            
            fluidPatchPointsDisplOld = fluidPatchPointsDispl;
                  
            fluidPatchPointsDispl += fsiRelaxationFactor*fsiResidual;
          }
        else
          {
            if (couplingScheme == "Aitken")
              {
                fsiRelaxationFactor =
                  -fsiRelaxationFactor
                  *(
                    gSum
                    (
                     fsiResidualOld
                     &(fsiResidual - fsiResidualOld)
                     )
                    /(
                      gSum
                      (
                       (fsiResidual - fsiResidualOld)
                       &(fsiResidual - fsiResidualOld)
                       )
                      )
                    );
                      
                fsiRelaxationFactor = mag(fsiRelaxationFactor);
                      
                Info << "Current fsi under-relaxation factor (Aitken): "
                     << fsiRelaxationFactor << endl;
                      
                fluidPatchPointsDisplOld = fluidPatchPointsDispl;
                      
                fluidPatchPointsDispl +=
                  fsiRelaxationFactor*fsiResidual;
              }
          }
              
        {

          const vectorField& n =
            fluidsMesh.boundaryMesh()[fluidPatchID].pointNormals();
                  
          primitivePatchInterpolation patchInterpolator
            (
             fluidsMesh.boundaryMesh()[fluidPatchID]
             );
                  
          scalarField pointDeltaCoeffs =
            patchInterpolator.faceToPointInterpolate
            (
             fluidsMesh.boundary()[fluidPatchID].deltaCoeffs()
             );
                  
          scalar delta =
            gMax
            (
             mag
             (
              n
              & (
                 accumulatedFluidInterfaceDisplacement
                 + fluidPatchPointsDispl
                 - fluidPatchPointsDisplOld
                 )
              )
             *pointDeltaCoeffs
             );
                  
          Info << "Maximal accumulated displacement of interface points: "
               << delta << endl;
                  
          if(delta < interfaceDeformationLimit)
            {
              // Move only interface points
              // Masoud
              Info << "Moving only interface...";
              // Masoud : End
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (fluidPatchPointsDispl, pointI)
                {
                  //Info << "1. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;

                  newPoints[meshPoints[pointI]] +=
                    fluidPatchPointsDispl[pointI]
                    - fluidPatchPointsDisplOld[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              // Accumulate interface points displacement
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl
                - fluidPatchPointsDisplOld;
            }
          else
            {
              // Move whole fluid mesh
              // Masoud
              Info << "Moving the whole mesh...";
              // Masoud: End
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (accumulatedFluidInterfaceDisplacement, pointI)
                {
                  //Info << "2. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;
                  newPoints[meshPoints[pointI]] -=
                    accumulatedFluidInterfaceDisplacement[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl
                - fluidPatchPointsDisplOld;
                      
                      
              if (feMotionSolver)
                {
                  tetPointVectorField& motionU =
                    const_cast<tetPointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<tetPointVectorField>
                     (
                      "motionU"
                      )
                     );
                          
                  fixedValueTetPolyPatchVectorField& motionUFluidPatch =
                    refCast<fixedValueTetPolyPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                          
                  tetPolyPatchInterpolation tppi
                    (
                     refCast<const faceTetPolyPatch>(motionUFluidPatch.patch())
                     );
                          
                  motionUFluidPatch ==
                    tppi.pointToPointInterpolate
                    (
                     accumulatedFluidInterfaceDisplacement
                     /runTime.deltaT().value()
                     );
                }
              else if (fvMotionSolver)
                {
                  pointVectorField& motionU =
                    const_cast<pointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<pointVectorField>
                     (
                      "pointMotionU"
                      )
                     );
                
                  fixedValuePointPatchVectorField& motionUFluidPatch =
                    refCast<fixedValuePointPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                
                  motionUFluidPatch ==
                    accumulatedFluidInterfaceDisplacement
                    /runTime.deltaT().value();
                }
              else
                {
                  FatalErrorIn(args.executable())
                    << "Problem with mesh motion solver selection"
                    << abort(FatalError);
                }
                      
              fluidsMesh.update();
                      
              accumulatedFluidInterfaceDisplacement = vector::zero;
            }
        }
                
        if(fluidsMesh.moving())
          {
            // Make the fluxes relative
            phi -= fvc::meshPhi(U);
          }
                  

        scalar CoNum = 0.0;
        scalar meanCoNum = 0.0;
        scalar velMag = 0.0;
                    
        if (fluidsMesh.nInternalFaces())
          {
            surfaceScalarField magPhi = mag(phi);
                        
            surfaceScalarField SfUfbyDelta =
              fluidsMesh.surfaceInterpolation::deltaCoeffs()*magPhi;
                        
            const scalar deltaT = runTime.deltaT().value();
                        
            CoNum = max(SfUfbyDelta/fluidsMesh.magSf()).value()*deltaT;
                        
            meanCoNum = (sum(SfUfbyDelta)/sum(fluidsMesh.magSf())).value()*deltaT;
                        
            velMag = max(magPhi/fluidsMesh.magSf()).value();
          }
                    
        Info<< "Courant Number mean: " << meanCoNum
            << " max: " << CoNum
            << " velocity magnitude: " << velMag
            << endl;
                    
        fvVectorMatrix UEqn
          (
           fvm::ddt(U)
           + fvm::div(phi, U)
           - fvm::laplacian(nu, U)
           );
                  
        solve(UEqn == -fvc::grad(p));
                  
        // --- PISO loop
        volScalarField rUA = 1.0/UEqn.A();
        // Masoud
        Info << "Performing " << nCorrPISO << " Pressure corrections." << endl;
        // Masoud End
                  
        for (int corr=0; corr<nCorrPISO; corr++)
          {
            U = rUA*UEqn.H();
            phi = (fvc::interpolate(U) & fluidsMesh.Sf());
                      
            adjustPhi(phi, U, p);
                      
            for (int nonOrth=0; nonOrth<=nNonOrthCorr; nonOrth++)
              {
                fvScalarMatrix pEqn
                  (
                   fvm::laplacian(rUA, p)
                   == fvc::div(phi)
                   );
                          
                pEqn.setReference(pRefCell, pRefValue);
                pEqn.solve();

                // Added by Masoud
                //Info << "p = " << (this-> p()) << endl;
                          
                if (nonOrth == nNonOrthCorr)
                  {
                    phi -= pEqn.flux();
                  }
              }
                      
            {
              volScalarField contErr = fvc::div(phi);
                        
              sumLocalContErr = runTime.deltaT().value()*
                mag(contErr)().weightedAverage(fluidsMesh.V()).value();
                        
              globalContErr = runTime.deltaT().value()*
                contErr.weightedAverage(fluidsMesh.V()).value();
                        
              cumulativeContErr += globalContErr;
                        
              Info<< "time step continuity errors : sum local = " << sumLocalContErr
                  << ", global = " << globalContErr
                  << ", cumulative = " << cumulativeContErr
                  << endl;
            }
                      
            U -= rUA*fvc::grad(p);
            U.correctBoundaryConditions();
          }
               
        {
          Info << "Not Setting traction on solid patch" << endl;
                      
          {
                          
            fsiResidualOld = fsiResidual;
            // Original: a fixed displacement delta will be used for the solid
            //this->UpdateFSISurface(solidPatchPointsDispl);
            // Oridinal : End
            
            // Masoud : in the initial step displacement delta will be added
            //          and for the rest of the process, it will be consider-
            //          as zeros vector.
            if (outerCorr == 1)
              {
                this->UpdateFSISurface(solidPatchPointsDispl);
              } else {
                  solidPatchPointsDispl = vector::zero;
                  //fluidPatchPointsDispl = vector::zero;
              }
            // Masoud : End
            
 	    // Masoud
            Info << "solidPatchPointsDispl = " << solidPatchPointsDispl << endl;
            //Masoud end
            
            // Original (solidPatchPointsDispl won't change by iteration
            // therefore huge pressure build-up happens) 
            fsiResidual = (solidPatchPointsDispl - fluidPatchPointsDispl);                          
            // Original end

                          
            fsiResidualNorm = ::sqrt(gSum(magSqr(fsiResidual)));
                          
            if (outerCorr == 1)
              {
                initialFsiResidualNorm = fsiResidualNorm;
              }
                          
            fsiResidualNorm /= initialFsiResidualNorm + SMALL;
                          
            Info << "Current fsi residual norm: " << fsiResidualNorm << endl;
          }
        }
      }
    while
      (
       (fsiResidualNorm > outerCorrTolerance)
       && (outerCorr < nOuterCorr)  //Masoud changed from outerCorr < nOuterCorr
       );
    
    
    if (runTime.outputTime())
      {
        runTime.write();
      }
  }
  return(0);  
}

// Masoud: This is simple iteration-less scheme
int fsifoam_module::StepFluidNonItr(){

  Foam::argList &args(this->ArgList());
  Foam::Time &runTime(this->RunTime());
  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  //  IOdictionary &transportProperties(this->TransportProperties());
  dimensionedScalar &nu(this->nu());
  dimensionedScalar &rhoFluid(this->rhoFluid());
  volScalarField &p(this->p());
  volVectorField &U(this->U());
  surfaceScalarField &phi(this->phi());
    
    
  // //  IOdictionary &couplingProperties(this->CouplingProperties());

  label fluidPatchID(this->FluidPatchID());
  label fluidZoneID(this->FluidZoneID());
  label solidZoneID(this->SolidZoneID());
    
  bool feMotionSolver(this->FEMotion());
  bool fvMotionSolver(this->FVMotion());
  vectorField &accumulatedFluidInterfaceDisplacement(this->AccumulatedFluidInterfaceDisplacements());
  //Info << "Checking AccumulatedFluidInterfaceDisplacementi = " << endl;
  //Info << accumulatedFluidInterfaceDisplacement;
  solidTractionFvPatchVectorField &tForce(this->tForce());
    
  zoneToZoneInterpolation &interpolatorFluidSolid(this->interpFluidSolid());
  zoneToZoneInterpolation &interpolatorSolidFluid(this->interpSolidFluid());
    
  scalar &sumLocalContErr(this->LocalContErr());
  scalar &globalContErr(this->GlobalContErr());
  scalar &cumulativeContErr(this->CumulativeContErr());
    
  labelList &globalFaceZones(this->GlobalFaceZones());
  labelListList &globalToLocalFaceZonePointMap(this->GlobalToLocalFaceZonePointMap());
  runTime++;
  if(!runTime.end()){
      
    this->ReadPISOControls();
    this->ReadFSIControls();
    word couplingScheme(this->CouplingScheme());
    //        label &outerCorr(this->OuterCorr());
    scalar &fsiRelaxationFactor(this->FSIRelaxationFactor());
    //        scalar &fsiRelaxationFactorMin(this->FSIRelaxationFactorMin());
    Switch fsi(this->FSIEnabled());
    int &nCorrPISO(this->NCorrPISO());
    int &nNonOrthCorr(this->NNonOrthCorr());
    scalar &interfaceDeformationLimit(this->InterfaceDeformationLimit());
    scalar &outerCorrTolerance(this->OuterCorrTolerance());
    int &nOuterCorr(this->NOuterCorr());
      
    vectorField fluidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fluidPatchPointsDisplOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField solidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidual
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidualOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
        
    scalar initialFsiResidualNorm = 0;
    scalar fsiResidualNorm = 0;        
    label outerCorr=0;

    do
      {
        outerCorr++;
        Info << "FsiFoam:StepFluidNonItr: outerCorr = " << outerCorr << endl;
        
        //#           include "setInterfaceDisplacement.H"
        Info << "FsiFoam:StepFluidNonItr: Time = " << runTime.timeName()
             << ", iteration: " << outerCorr << endl;
        // Updating fluid patch displacement        
        this->UpdateFSISurface(fluidPatchPointsDispl);
        //Info << " Updating fluid interface coordinates with : " << endl;
        //Info << fluidPatchPointsDispl << endl;
        {

          const vectorField& n =
            fluidsMesh.boundaryMesh()[fluidPatchID].pointNormals();
                  
          primitivePatchInterpolation patchInterpolator
            (
             fluidsMesh.boundaryMesh()[fluidPatchID]
             );
                  
          scalarField pointDeltaCoeffs =
            patchInterpolator.faceToPointInterpolate
            (
             fluidsMesh.boundary()[fluidPatchID].deltaCoeffs()
             );
                  
          scalar delta =
            gMax
            (
             mag
             (
              n
              & (
                 accumulatedFluidInterfaceDisplacement
                 + fluidPatchPointsDispl
                 )
              )
             *pointDeltaCoeffs
             );
                  
          Info << "FsiFoam:StepFluidNonItr: Maximal accumulated displacement of interface points: "
               << delta << endl;
                  
          if(delta < interfaceDeformationLimit)
            {
              // Move only interface points
              Info << "FsiFoam:StepFluidNonItr: Moving only interface...";
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (fluidPatchPointsDispl, pointI)
                {
                  //Info << "1. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;

                  newPoints[meshPoints[pointI]] +=
                    fluidPatchPointsDispl[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              // Accumulate interface points displacement
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl;
            }
          else
            {
              // Move whole fluid mesh
              Info << "FsiFoam:StepFluidNonItr: Moving the whole mesh...";
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (accumulatedFluidInterfaceDisplacement, pointI)
                {
                  //Info << "2. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;
                  newPoints[meshPoints[pointI]] -=
                    accumulatedFluidInterfaceDisplacement[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl;
                      
              if (feMotionSolver)
                {
                  tetPointVectorField& motionU =
                    const_cast<tetPointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<tetPointVectorField>
                     (
                      "motionU"
                      )
                     );
                          
                  fixedValueTetPolyPatchVectorField& motionUFluidPatch =
                    refCast<fixedValueTetPolyPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                          
                  tetPolyPatchInterpolation tppi
                    (
                     refCast<const faceTetPolyPatch>(motionUFluidPatch.patch())
                     );
                          
                  motionUFluidPatch ==
                    tppi.pointToPointInterpolate
                    (
                     accumulatedFluidInterfaceDisplacement
                     /runTime.deltaT().value()
                     );
                }
              else if (fvMotionSolver)
                {
                  pointVectorField& motionU =
                    const_cast<pointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<pointVectorField>
                     (
                      "pointMotionU"
                      )
                     );
                
                  fixedValuePointPatchVectorField& motionUFluidPatch =
                    refCast<fixedValuePointPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                
                  motionUFluidPatch ==
                    accumulatedFluidInterfaceDisplacement
                    /runTime.deltaT().value();
                }
              else
                {
                  FatalErrorIn(args.executable())
                    << "FsiFoam:StepFluidNonItr: Problem with mesh motion solver selection"
                    << abort(FatalError);
                }
              

              Info << "FsiFoam:StepFluidNonItr: runTime.deltaT()  = "
              << runTime.deltaT().value() << endl;

    
              fluidsMesh.update();
                      
              accumulatedFluidInterfaceDisplacement = vector::zero;
            }
        }
                
        if(fluidsMesh.moving())
          {
            // Make the fluxes relative
            Info << "Compensating for mesh movment !" << endl;
            phi -= fvc::meshPhi(U);
          }
                  

        scalar CoNum = 0.0;
        scalar meanCoNum = 0.0;
        scalar velMag = 0.0;
                    
        if (fluidsMesh.nInternalFaces())
          {
            surfaceScalarField magPhi = mag(phi);
                        
            surfaceScalarField SfUfbyDelta =
              fluidsMesh.surfaceInterpolation::deltaCoeffs()*magPhi;
                        
            const scalar deltaT = runTime.deltaT().value();
                        
            CoNum = max(SfUfbyDelta/fluidsMesh.magSf()).value()*deltaT;
                        
            meanCoNum = (sum(SfUfbyDelta)/sum(fluidsMesh.magSf())).value()*deltaT;
                        
            velMag = max(magPhi/fluidsMesh.magSf()).value();
          }
                    
        Info<< "FsiFoam:StepFluidNonItr: Courant Number mean: " << meanCoNum
            << " max: " << CoNum
            << " velocity magnitude: " << velMag
            << endl;
                    
        fvVectorMatrix UEqn
          (
           fvm::ddt(U)
           + fvm::div(phi, U)
           - fvm::laplacian(nu, U)
           );
                  
        solve(UEqn == -fvc::grad(p));
                  
        // --- PISO loop
        volScalarField rUA = 1.0/UEqn.A();
        //Info << "nCorrPISO = " << nCorrPISO << endl;
        //Info << "nNonOrthCorr = " << nNonOrthCorr << endl;
        for (int corr=0; corr<nCorrPISO; corr++)
          {
            U = rUA*UEqn.H();
            phi = (fvc::interpolate(U) & fluidsMesh.Sf());
                      
            adjustPhi(phi, U, p);
                      
            for (int nonOrth=0; nonOrth<=nNonOrthCorr; nonOrth++)
              {
                fvScalarMatrix pEqn
                  (
                   fvm::laplacian(rUA, p)
                   == fvc::div(phi)
                   );
                          
                pEqn.setReference(pRefCell, pRefValue);
                pEqn.solve();

                //Info << "p = " << p << endl;
                          
                if (nonOrth == nNonOrthCorr)
                  {
                    phi -= pEqn.flux();
                  }
              }
                      
            {
              volScalarField contErr = fvc::div(phi);
                        
              sumLocalContErr = runTime.deltaT().value()*
                mag(contErr)().weightedAverage(fluidsMesh.V()).value();
                        
              globalContErr = runTime.deltaT().value()*
                contErr.weightedAverage(fluidsMesh.V()).value();
                        
              cumulativeContErr += globalContErr;
                        
              Info<< "FsiFoam:StepFluidNonItr: time step continuity errors : sum local = " << sumLocalContErr
                  << ", global = " << globalContErr
                  << ", cumulative = " << cumulativeContErr
                  << endl;
            }
                      
            U -= rUA*fvc::grad(p);
            U.correctBoundaryConditions();
          }
               
      }
    while
      ( outerCorr < 1 );
    
    
    if (runTime.outputTime())
      {
        runTime.write();
      }
  }
  return(0);  
}
//Masoud End (StepFluidNonItr)


//Masoud: StepFluidItr
int fsifoam_module::StepFluidItr(){

  Foam::argList &args(this->ArgList());
  Foam::Time &runTime(this->RunTime());
  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  //  IOdictionary &transportProperties(this->TransportProperties());
  dimensionedScalar &nu(this->nu());
  dimensionedScalar &rhoFluid(this->rhoFluid());
  volScalarField &p(this->p());
  volVectorField &U(this->U());
  surfaceScalarField &phi(this->phi());
    
    
  // //  IOdictionary &couplingProperties(this->CouplingProperties());

  label fluidPatchID(this->FluidPatchID());
  label fluidZoneID(this->FluidZoneID());
  label solidZoneID(this->SolidZoneID());
    
  bool feMotionSolver(this->FEMotion());
  bool fvMotionSolver(this->FVMotion());
  vectorField &accumulatedFluidInterfaceDisplacement(this->AccumulatedFluidInterfaceDisplacements());
  // Masoud : Checking AccumulatedFluidInterfaceDisplacement
  Info << "Checking AccumulatedFluidInterfaceDisplacementi = " << endl;
  Info << accumulatedFluidInterfaceDisplacement;
  // Masoud: End
  solidTractionFvPatchVectorField &tForce(this->tForce());
    
  zoneToZoneInterpolation &interpolatorFluidSolid(this->interpFluidSolid());
  zoneToZoneInterpolation &interpolatorSolidFluid(this->interpSolidFluid());
    
  scalar &sumLocalContErr(this->LocalContErr());
  scalar &globalContErr(this->GlobalContErr());
  scalar &cumulativeContErr(this->CumulativeContErr());
    
  labelList &globalFaceZones(this->GlobalFaceZones());
  labelListList &globalToLocalFaceZonePointMap(this->GlobalToLocalFaceZonePointMap());
  runTime++;
  if(!runTime.end()){
      
    this->ReadPISOControls();
    this->ReadFSIControls();
    word couplingScheme(this->CouplingScheme());
    //        label &outerCorr(this->OuterCorr());
    scalar &fsiRelaxationFactor(this->FSIRelaxationFactor());
    //        scalar &fsiRelaxationFactorMin(this->FSIRelaxationFactorMin());
    Switch fsi(this->FSIEnabled());
    int &nCorrPISO(this->NCorrPISO());
    int &nNonOrthCorr(this->NNonOrthCorr());
    scalar &interfaceDeformationLimit(this->InterfaceDeformationLimit());
    scalar &outerCorrTolerance(this->OuterCorrTolerance());
    int &nOuterCorr(this->NOuterCorr());
      
    vectorField fluidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fluidPatchPointsDisplOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField solidPatchPointsDispl
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidual
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
    vectorField fsiResidualOld
      (
       fluidsMesh.boundaryMesh()[fluidPatchID].nPoints(),
       vector::zero
       );
        
        
    scalar initialFsiResidualNorm = 0;
    scalar fsiResidualNorm = 0;        
    label outerCorr=0;

    do
      {
        outerCorr++;
        Info << "outerCorr = " << outerCorr << endl;
        
        //#           include "setInterfaceDisplacement.H"
        Info << "\nTime = " << runTime.timeName()
             << ", iteration: " << outerCorr << endl;
        
        if (outerCorr < 3 || couplingScheme == "FixedRelaxation")
          {
            Info << "Current fsi under-relaxation factor: "
                 << fsiRelaxationFactor << endl;
            
            fluidPatchPointsDisplOld = fluidPatchPointsDispl;
                  
            fluidPatchPointsDispl += fsiRelaxationFactor*fsiResidual;
          }
        else
          {
            if (couplingScheme == "Aitken")
              {
                fsiRelaxationFactor =
                  -fsiRelaxationFactor
                  *(
                    gSum
                    (
                     fsiResidualOld
                     &(fsiResidual - fsiResidualOld)
                     )
                    /(
                      gSum
                      (
                       (fsiResidual - fsiResidualOld)
                       &(fsiResidual - fsiResidualOld)
                       )
                      )
                    );
                      
                fsiRelaxationFactor = mag(fsiRelaxationFactor);
                      
                Info << "Current fsi under-relaxation factor (Aitken): "
                     << fsiRelaxationFactor << endl;
                      
                fluidPatchPointsDisplOld = fluidPatchPointsDispl;
                      
                fluidPatchPointsDispl +=
                  fsiRelaxationFactor*fsiResidual;
              }
          }
              
        {

          const vectorField& n =
            fluidsMesh.boundaryMesh()[fluidPatchID].pointNormals();
                  
          primitivePatchInterpolation patchInterpolator
            (
             fluidsMesh.boundaryMesh()[fluidPatchID]
             );
                  
          scalarField pointDeltaCoeffs =
            patchInterpolator.faceToPointInterpolate
            (
             fluidsMesh.boundary()[fluidPatchID].deltaCoeffs()
             );
                  
          scalar delta =
            gMax
            (
             mag
             (
              n
              & (
                 accumulatedFluidInterfaceDisplacement
                 + fluidPatchPointsDispl
                 - fluidPatchPointsDisplOld
                 )
              )
             *pointDeltaCoeffs
             );
                  
          Info << "Maximal accumulated displacement of interface points: "
               << delta << endl;
                  
          if(delta < interfaceDeformationLimit)
            {
              // Move only interface points
              // Masoud
              Info << "Moving only interface...";
              // Masoud : End
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (fluidPatchPointsDispl, pointI)
                {
                  //Info << "1. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;

                  newPoints[meshPoints[pointI]] +=
                    fluidPatchPointsDispl[pointI]
                    - fluidPatchPointsDisplOld[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              // Accumulate interface points displacement
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl
                - fluidPatchPointsDisplOld;
            }
          else
            {
              // Move whole fluid mesh
              // Masoud
              Info << "Moving the whole mesh...";
              // Masoud: End
              pointField newPoints = fluidsMesh.allPoints();
                      
              const labelList& meshPoints =
                fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
                      
              forAll (accumulatedFluidInterfaceDisplacement, pointI)
                {
                  //Info << "2. meshPoints[" << pointI << "] = " << meshPoints[pointI]
                       //<< endl;
                  newPoints[meshPoints[pointI]] -=
                    accumulatedFluidInterfaceDisplacement[pointI];
                }
                      
              twoDPointCorrector twoDCorrector(fluidsMesh);
                      
              twoDCorrector.correctPoints(newPoints);
                      
              fluidsMesh.movePoints(newPoints);
                      
              accumulatedFluidInterfaceDisplacement +=
                fluidPatchPointsDispl
                - fluidPatchPointsDisplOld;
                      
                      
              if (feMotionSolver)
                {
                  tetPointVectorField& motionU =
                    const_cast<tetPointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<tetPointVectorField>
                     (
                      "motionU"
                      )
                     );
                          
                  fixedValueTetPolyPatchVectorField& motionUFluidPatch =
                    refCast<fixedValueTetPolyPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                          
                  tetPolyPatchInterpolation tppi
                    (
                     refCast<const faceTetPolyPatch>(motionUFluidPatch.patch())
                     );
                          
                  motionUFluidPatch ==
                    tppi.pointToPointInterpolate
                    (
                     accumulatedFluidInterfaceDisplacement
                     /runTime.deltaT().value()
                     );
                }
              else if (fvMotionSolver)
                {
                  pointVectorField& motionU =
                    const_cast<pointVectorField&>
                    (
                     fluidsMesh.objectRegistry::
                     lookupObject<pointVectorField>
                     (
                      "pointMotionU"
                      )
                     );
                
                  fixedValuePointPatchVectorField& motionUFluidPatch =
                    refCast<fixedValuePointPatchVectorField>
                    (
                     motionU.boundaryField()[fluidPatchID]
                     );
                
                  motionUFluidPatch ==
                    accumulatedFluidInterfaceDisplacement
                    /runTime.deltaT().value();
                }
              else
                {
                  FatalErrorIn(args.executable())
                    << "Problem with mesh motion solver selection"
                    << abort(FatalError);
                }
                      
              fluidsMesh.update();
                      
              accumulatedFluidInterfaceDisplacement = vector::zero;
            }
        }
                
        if(fluidsMesh.moving())
          {
            // Make the fluxes relative
            phi -= fvc::meshPhi(U);
          }
                  

        scalar CoNum = 0.0;
        scalar meanCoNum = 0.0;
        scalar velMag = 0.0;
                    
        if (fluidsMesh.nInternalFaces())
          {
            surfaceScalarField magPhi = mag(phi);
                        
            surfaceScalarField SfUfbyDelta =
              fluidsMesh.surfaceInterpolation::deltaCoeffs()*magPhi;
                        
            const scalar deltaT = runTime.deltaT().value();
                        
            CoNum = max(SfUfbyDelta/fluidsMesh.magSf()).value()*deltaT;
                        
            meanCoNum = (sum(SfUfbyDelta)/sum(fluidsMesh.magSf())).value()*deltaT;
                        
            velMag = max(magPhi/fluidsMesh.magSf()).value();
          }
                    
        Info<< "Courant Number mean: " << meanCoNum
            << " max: " << CoNum
            << " velocity magnitude: " << velMag
            << endl;
                    
        fvVectorMatrix UEqn
          (
           fvm::ddt(U)
           + fvm::div(phi, U)
           - fvm::laplacian(nu, U)
           );
                  
        solve(UEqn == -fvc::grad(p));
                  
        // --- PISO loop
        volScalarField rUA = 1.0/UEqn.A();
        // Masoud
        Info << "Performing " << nCorrPISO << " Pressure corrections." << endl;
        // Masoud End
                  
        for (int corr=0; corr<nCorrPISO; corr++)
          {
            U = rUA*UEqn.H();
            phi = (fvc::interpolate(U) & fluidsMesh.Sf());
                      
            adjustPhi(phi, U, p);
                      
            for (int nonOrth=0; nonOrth<=nNonOrthCorr; nonOrth++)
              {
                fvScalarMatrix pEqn
                  (
                   fvm::laplacian(rUA, p)
                   == fvc::div(phi)
                   );
                          
                pEqn.setReference(pRefCell, pRefValue);
                pEqn.solve();

                // Added by Masoud
                //Info << "p = " << (this-> p()) << endl;
                          
                if (nonOrth == nNonOrthCorr)
                  {
                    phi -= pEqn.flux();
                  }
              }
                      
            {
              volScalarField contErr = fvc::div(phi);
                        
              sumLocalContErr = runTime.deltaT().value()*
                mag(contErr)().weightedAverage(fluidsMesh.V()).value();
                        
              globalContErr = runTime.deltaT().value()*
                contErr.weightedAverage(fluidsMesh.V()).value();
                        
              cumulativeContErr += globalContErr;
                        
              Info<< "time step continuity errors : sum local = " << sumLocalContErr
                  << ", global = " << globalContErr
                  << ", cumulative = " << cumulativeContErr
                  << endl;
            }
                      
            U -= rUA*fvc::grad(p);
            U.correctBoundaryConditions();
          }
               
        {
          Info << "Setting tractionis on solid patch" << endl;
                      
          {
                           
            fsiResidualOld = fsiResidual;
            this->UpdateFSISurface(solidPatchPointsDispl);
            Info << "solidPatchPointsDispl = " << solidPatchPointsDispl << endl;
            
            fsiResidual = (solidPatchPointsDispl - fluidPatchPointsDispl);                          
            fsiResidualNorm = ::sqrt(gSum(magSqr(fsiResidual)));
                          
            if (outerCorr == 1)
              {
                initialFsiResidualNorm = fsiResidualNorm;
              }
                          
            fsiResidualNorm /= initialFsiResidualNorm + SMALL;
                          
            Info << "Current fsi residual norm: " << fsiResidualNorm << endl;
          }
        }
      }
    while
      (
       (fsiResidualNorm > outerCorrTolerance)
       && (outerCorr < nOuterCorr)  //Masoud changed from outerCorr < nOuterCorr
       );
    
    
    if (runTime.outputTime())
      {
        runTime.write();
      }
  }
  return(0);  
}
//Masoud End StepFluidItr

int fsifoam_module::Dump(){
  Foam::Time &runTime(this->RunTime());
  fvMesh &structuresMesh(this->StructuresMesh());
  volSymmTensorField &sigma(this->sigma());
  volScalarField sigmaEq
    (
     IOobject
     (
      "sigmaEq",
      runTime.timeName(),
      structuresMesh,
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
      ),
     sqrt((3.0/2.0)*magSqr(dev(sigma)))
     );
    
  Info<< "Max sigmaEq = " << max(sigmaEq).value()
      << endl;
    
  runTime.write();
  return 0;
};

///
/// @brief "Loads" IcoFoamModule
///
///
//static void fsifoam_module::Load(const std::string &name){
void fsifoam_module::Load(const std::string &name){
  std::cout << "Loading FsiFoamModule with name " << name 
            << "." << std::endl;

  /// Register module with COM
  fsifoam_module *module_pointer = new fsifoam_module();
  COM_new_window(name, MPI_COMM_NULL);
  module_pointer->my_window_name = name;
  std::string global_name(name+".global");
  COM_new_dataitem(global_name.c_str(),'w',COM_VOID,1,"");
  COM_set_object(global_name.c_str(),0,module_pointer);


  /// Register functions
  std::vector<COM_Type> types(13,COM_INT);

  types[0] = COM_RAWDATA;
  types[2] = COM_VOID;
  COM_set_member_function( (name+".InitFoam").c_str(),
                           (Member_func_ptr)(&fsifoam_module::InitFoam),
                           global_name.c_str(), "bii", &types[0]);

  COM_set_member_function( (name+".RunFoam").c_str(),
                           (Member_func_ptr)(&fsifoam_module::RunFoam),
                           global_name.c_str(), "b", &types[0]);


  COM_set_member_function( (name+".StepFoam").c_str(),
                           (Member_func_ptr)(&fsifoam_module::StepFoam),
                           global_name.c_str(), "b", &types[0]);

  COM_set_member_function( (name+".StepFluid").c_str(),
                           (Member_func_ptr)(&fsifoam_module::StepFluid),
                           global_name.c_str(), "b", &types[0]);

/*
  types[1] = COM_DOUBLE;
  COM_set_member_function( (name+".ModifyEndTime").c_str(),
                           (Member_func_ptr)(&fsifoam_module::ModifyEndTime),
                           global_name.c_str(), "bi", &types[0]);
*/
  

  COM_window_init_done(name);  
}


void fsifoam_module::InitFoam(int *pargc, void **pargv)
{
  int argc = *pargc;
  char** argv = (char**)(pargv);

/*
  // register function status variables
  std::string initStatusName(my_window_name+".initStatus");
  COM_new_dataitem(initStatusName.c_str(),'w', COM_INT, 1, "");
  COM_set_array(initStatusName.c_str(), 11, &initStatus);
*/

  Initialize(argc, argv);

  /// Register the Data Items

  // set up solution meta data
  Solution().Meta().AddField("time", 's', 1, 8, "s");
  Solution().Meta().AddField("endTime", 's', 1, 8, "s");
  Solution().Meta().AddField("initStatus", 's', 1, 4, "");
  Solution().Meta().AddField("runStatus", 's', 1, 4, "");
  Solution().Meta().AddField("dt", 's', 1, 8, "s");
  Solution().Meta().AddField("solidDisplacement", 'n', 3, 8, "m");
  Solution().Meta().AddField("pressure", 'c', 1, 8, "Pa");
  Solution().Meta().AddField("traction", 'c', 3, 8, "N");
 
  Mesh().nc.init(numPointsSurface, &surfaceCoordinates[0]);
  // this assumes that our elements are quads...we can generalize this
  Mesh().con.AddElements(numElementsSurface, 4, surfaceConnectivity);

  // create buffers for the actual data
  CreateSoln();

  //unsigned int nnodes = Mesh().nc.NNodes();
  //unsigned int nelem = Mesh().con.Nelem();

  // allocate our own local buffers for the data

  time.resize(1, -1.0);
  endTime.resize(1, -1.0);
  UpdateTime();

  initStatus.resize(1, -1000);
  runStatus.resize(1, -1000);

  surfacePressure.resize(numElementsSurface, -1);
  surfaceTraction.resize(3*numElementsSurface, -1);
  solidDisplacement.resize(3*numPointsSurface,0.);

  // reset the buffers to be our own local buffers
  Solution().SetFieldBuffer("initStatus", initStatus);
  Solution().SetFieldBuffer("runStatus", runStatus);
  Solution().SetFieldBuffer("time", time);
  Solution().SetFieldBuffer("endTime", endTime);
  Solution().SetFieldBuffer("pressure", surfacePressure);
  Solution().SetFieldBuffer("traction", surfaceTraction);
  Solution().SetFieldBuffer("solidDisplacement", solidDisplacement);
 
  SolverUtils::RegisterSolverInto(my_window_name.c_str(), *this);

  initStatus[0] = 0;
  return;
} 


///
/// @brief Unloads the IcoFoamModule
///
///
//static void fsifoam_module::Unload(const std::string &name){
void fsifoam_module::Unload(const std::string &name){
  std::cout << "Unloading FsiFoamModule with name " << name 
            << "." << std::endl;
  fsifoam_module *module_pointer = NULL;
  std::string global_name(name+".global");
  COM_get_object(global_name.c_str(),0,&module_pointer);
  COM_assertion_msg( module_pointer->validate_object()==0, "Invalid object");
  delete module_pointer;
  COM_delete_window(std::string(name));
}

///
/// @encapsulate the OpenFOAM main
///
void fsifoam_module::RunFoam(){


  Foam::Time &runTime(RunTime());

  Info << "\nStarting time loop\n" << endl;

  while(!runTime.end()){
    Info << "Time = " << runTime.timeName() << nl << endl;
    Step();
    Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        << endl << endl;
    if (runTime.outputTime())
      Dump();
  }

  Info<< "End\n" << endl;
  runStatus[0] = 0;

  return;
}

///
/// @encapsulate the OpenFOAM stepping
///
void fsifoam_module::StepFoam(){


  Foam::Time &runTime(RunTime());

  Info << "\nStepping time loop\n" << endl;

  //  while(!runTime.end()){
  if(!runTime.end()){
    Info << "Time = " << runTime.timeName() << nl << endl;
    Step();
    Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        << endl << endl;
    if (runTime.outputTime())
      Dump();

    // update IMPACT time
    UpdateTime();
    UpdateFSISurfaceData();
    //Masoud
    UpdateFSISurfaceMesh();
    //Masoud : End

    // Masoud : cheking impact data
    Info << "numPointSurface = " << numPointsSurface << endl;
    Info << "numElementSurface = " << numElementsSurface << endl;
    Info << "SurfaceCoordinates = " << endl;
    for (int i = 0; i < numPointsSurface; i++)
    {
	Info << surfaceCoordinates[i*3] << " "
             << surfaceCoordinates[i*3+1] << " "
             << surfaceCoordinates[i*3+2] << endl;
    }

    // Masoud : end
  }
  if(runTime.end()){
    Info<< "End\n" << endl;
  }
  runStatus[0] = 0;
  return;
}

///
/// @encapsulate the OpenFOAM stepping fluid alone
///
void fsifoam_module::StepFluid(){

  // Masoud: probe output file and preps
  ofstream prb2DmpFile;
  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  pointField foamCoords = fluidsMesh.allPoints();
  // Masoud: end

  Foam::Time &runTime(RunTime());

  Info << "FsiFoam:StepFluid: Stepping fluid solver..." << endl;

  //  while(!runTime.end()){
  if(!runTime.end()){
    Info << "FsiFoam:StepFluid: Time = " << runTime.timeName() << endl;
    // Original: This step routine does perform iterations properly
    //  StepFluidAlone();
    // Original: End
    
    // Masoud: Non-iterative stepping
     StepFluidNonItr();
    // Masoud: end

    // Masoud: Iterative stepping
    //StepFluidItr();
    // Masoud: end
    
    Info<< "FsiFoam:StepFluid: ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        << endl;
    if (runTime.outputTime())
      Dump();

    // update IMPACT data
    UpdateTime();
    UpdateFSISurfaceData(); // Masoud: updates tractions and pressures
    UpdateFSISurfaceMesh(); // Masoud: updates surface mesh

    // Masoud : cheking impact data
    //Info << "numPointSurface = " << numPointsSurface << endl;
    //Info << "numElementSurface = " << numElementsSurface << endl;
    //Info << "SurfaceCoordinates = " << endl;
    //for (int i = 0; i < numPointsSurface; i++)
    //{
    //	Info << surfaceCoordinates[i*3] << " "
    //         << surfaceCoordinates[i*3+1] << " "
    //         << surfaceCoordinates[i*3+2] << endl;
    //}
    // Masoud : end

    // Masoud: output for probe
    prb2DmpFile.open("prb2.dat", std::ios::app);
    prb2DmpFile <<  runTime.timeName()  << " " << \
    foamCoords[64].x() << " " << \
    foamCoords[64].y() << " " << \
    foamCoords[64].z() << "\n"; 
    prb2DmpFile.close();
    // Masoud: end

  }
  if(runTime.end()){
    Info<< "FsiFoam:StepFluid: End\n" << endl;
  }
  runStatus[0] = 0;
  return;
}

/*
void fsifoam_module::ModifyEndTime(const double &endTime){
 RunTime().setEndTime(endTime); 
}
*/

void fsifoam_module::CreateFSISurfaceMesh(){

  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  numPointsSurface = fluidsMesh.boundaryMesh()[fluidPatchID].nPoints();
  //numElementsSurface = fluidsMesh.boundaryMesh()[fluidPatchID].faceCentres().nPoints();

  // face storage
  const faceList& faces = fluidsMesh.faces();
  const labelList& owner = fluidsMesh.faceOwner();
  const polyBoundaryMesh& boundaryMesh = fluidsMesh.boundaryMesh();
  const label start = boundaryMesh[fluidPatchID].start();
  const label end = start + boundaryMesh[fluidPatchID].size();
  numElementsSurface = end - start;

  // point storage
  const pointField points = fluidsMesh.points();

  // node 1 and element 1 live at index zero
  int nodeNumber = 1;
  for (label faceI = start; faceI < end; ++faceI) {
    const labelList& vrtList = faces[faceI];
    forAll(vrtList, i) {
      const label& vertex = vrtList[i];

      // create forward and reverse mapping for this vertex if not already in map
      if (!surfaceNodeMap[vertex]) {
        surfaceNodeMap[vertex] = nodeNumber;
        interfaceToFoamNodeMap[nodeNumber] = vertex;
        nodeNumber++;
        // add to node list
        surfaceCoordinates.push_back(points[vertex].x()); 
        surfaceCoordinates.push_back(points[vertex].y()); 
        surfaceCoordinates.push_back(points[vertex].z()); 
      }
      // build local element connectivity
      surfaceConnectivity.push_back(vertex);
    }
  }

  // build map for OpenFoam global vertices to OpenFoam patch vertices
  const labelList& meshPoints = 
    fluidsMesh.boundaryMesh()[fluidPatchID].meshPoints();
  for (int i=0; i<numPointsSurface; ++i) {
    foamGlobalToPatchNodeMap[meshPoints[i]] = i;
  }

  // translate OpenFOAM global node numbers to local surface node numbers
  // node 1 and element 1 live at index zero
  for (unsigned int i=0; i<surfaceConnectivity.size(); ++i) {
    surfaceConnectivity[i] = surfaceNodeMap[surfaceConnectivity[i]];
  }
}

void fsifoam_module::UpdateFSISurfaceMesh(){

  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  pointField foamCoords = fluidsMesh.allPoints();
   
  for (int i=0; i<numPointsSurface; ++i) {
    int vertex = interfaceToFoamNodeMap[i+1];
    surfaceCoordinates[3*i] = foamCoords[vertex].x();
    surfaceCoordinates[3*i+1] = foamCoords[vertex].y();
    surfaceCoordinates[3*i+2] = foamCoords[vertex].z();
  }
}

void fsifoam_module::UpdateTime(){
  Foam::Time &runTime(this->RunTime());
  time[0] = runTime.value();
  endTime[0] = runTime.endTime().value();
}

int fsifoam_module::UpdateFSISurface(Foam::vectorField &solidDispl){
  //Masoud : Testing displacement
  std::cout << 
  "FsiFoam:UpdateFSISurface: Reporting solid displacements to the fluid solver" << std::endl;
  //std::cout<< "Displacements passed to me : " << std::endl;
  // Masoud : End
  /////////////////////////////
  // final displacement of the fluid surface 
  /////////////////////////////
  for(int i=0; i<numPointsSurface; ++i) {
    solidDispl[foamGlobalToPatchNodeMap[interfaceToFoamNodeMap[i+1]]].x() =
      solidDisplacement[3*i];
    solidDispl[foamGlobalToPatchNodeMap[interfaceToFoamNodeMap[i+1]]].y() =
      solidDisplacement[3*i+1];
    solidDispl[foamGlobalToPatchNodeMap[interfaceToFoamNodeMap[i+1]]].z() =
      solidDisplacement[3*i+2];
    //Masoud : Testing displacement
    //std::cout<< solidDisplacement[3*i] << " " 
    //         << solidDisplacement[3*i+1] << " "
    //         << solidDisplacement[3*i+2] << std::endl;
    //Masoud: End
  }
  return(0);
}

void fsifoam_module::UpdateFSISurfaceData(){

   //Masoud
   double currTime((this->RunTime()).value());
   Info << "FsiFoam:UpdateFSISurface: Reporting surface loads to the solid solver " << endl;

  /////////////////////////////
  // pressure on the surface
  /////////////////////////////
  dimensionedScalar &rhoFluid(this->rhoFluid());
  volScalarField &p(this->p());

  scalarField foamSurfacePressure = rhoFluid.value()*p.boundaryField()[fluidPatchID];

  for(int i=0; i<numElementsSurface; ++i) {
    surfacePressure[i] = foamSurfacePressure[i];
  }

  /////////////////////////////
  // traction on the surface
  /////////////////////////////
  dimensionedScalar &nu(this->nu());
  volVectorField &U(this->U());
  // Original
  //vectorField foamSurfaceTraction = -rhoFluid.value()*nu.value()*
  //  U.boundaryField()[fluidPatchID].snGrad();
  // Original end

  // Masoud change: based on following taken from StepFoam() 
          //     vectorField fluidPatchTraction =
          //        -rhoFluid.value()*nu.value()
          //        *U.boundaryField()[fluidPatchID].snGrad()
          //       + rhoFluid.value()*p.boundaryField()[fluidPatchID]
          //        *fluidsMesh.boundary()[fluidPatchID].nf();

  dynamicFvMesh &fluidsMesh(this->FluidMesh());
  vectorField foamSurfaceTraction = -rhoFluid.value()*nu.value()
      *U.boundaryField()[fluidPatchID].snGrad()
      + rhoFluid.value()*p.boundaryField()[fluidPatchID]
      *fluidsMesh.boundary()[fluidPatchID].nf();
  //Masoud: end
  
  //Masoud: sending tractions after some initial time
  if (currTime > -0.1) {
     for(int i=0; i<numElementsSurface; ++i) {
       surfaceTraction[3*i] = foamSurfaceTraction[i].x();
       surfaceTraction[3*i+1] = foamSurfaceTraction[i].y();
       surfaceTraction[3*i+2] = foamSurfaceTraction[i].z();
     }
  } else {
     for(int i=0; i<numElementsSurface; ++i) {
       surfaceTraction[3*i] = 0.0;
       surfaceTraction[3*i+1] = 0.0;
       surfaceTraction[3*i+2] = 0.0;
     }
  }
  //Masoud: end
}

/// 
/// @brief C/C++ bindings to load IcoFoamModule
///
extern "C" void OpenFoamFSI_load_module( const char *name) {
  fsifoam_module::Load(name);
}
///
/// @brief C/C++ bindings to unload IcoFoamModule
///
extern "C" void OpenFoamFSI_unload_module( const char *name){
  fsifoam_module::Unload(name);
}
