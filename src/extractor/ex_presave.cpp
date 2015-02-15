#include <iostream>
#include "io/GLDatasetBase.h"
#include "extractor/Extractor.h"

int main(int argc, char **argv)
{
  if (argc<4) return EXIT_FAILURE;

  const std::string dataname = argv[1];
  const int timestep0 = atoi(argv[2]), 
            ntimesteps = atoi(argv[3]);

  GLDatasetBase *ds = new GLDatasetBase;
  ds->SetDataName(dataname);
  ds->SetTimeStep(timestep0);
  ds->SetTimeStep1(timestep0+1);
  if (!ds->LoadDefaultMeshGraph()) return EXIT_FAILURE;

  VortexExtractor *extractor = new VortexExtractor;
  extractor->SetDataset(ds);
  if (!extractor->LoadPuncturedFaces(0)) return EXIT_FAILURE;
  if (!extractor->LoadPuncturedFaces(1)) return EXIT_FAILURE;
  if (!extractor->LoadPuncturedEdges()) return EXIT_FAILURE;

  fprintf(stderr, "tracing..\n");
  extractor->TraceOverSpace(0);
  extractor->TraceOverSpace(1);
  extractor->RelateOverTime();
  extractor->TraceOverTime();
  // extractor->SaveVortexLines(dataname + ".vortex");

  delete extractor;
  delete ds; 

  return EXIT_SUCCESS;
}


#if 0
int main(int argc, char **argv)
{
  if (argc<4) return EXIT_FAILURE;

  const std::string dataname = argv[1];
  const int timestep = atoi(argv[2]), 
            timestep1 = atoi(argv[3]);

  GLDatasetBase *ds = new GLDatasetBase;
  ds->SetDataName(dataname);
  ds->SetTimeSteps(timestep, timestep1);
  if (!ds->LoadDefaultMeshGraph()) return EXIT_FAILURE;

  VortexExtractor *extractor = new VortexExtractor;
  extractor->SetDataset(ds);
  if (!extractor->LoadPuncturedFaces(0)) return EXIT_FAILURE;
  if (!extractor->LoadPuncturedFaces(1)) return EXIT_FAILURE;
  if (!extractor->LoadPuncturedEdges()) return EXIT_FAILURE;

  fprintf(stderr, "tracing..\n");
  extractor->TraceOverSpace(0);
  extractor->TraceOverSpace(1);
  extractor->RelateOverTime();
  extractor->TraceOverTime();
  // extractor->SaveVortexLines(dataname + ".vortex");

  delete extractor;
  delete ds; 

  return EXIT_SUCCESS;
}
#endif
