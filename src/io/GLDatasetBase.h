#ifndef _GLDATASET_BASE_H
#define _GLDATASET_BASE_H

#include <string>
#include <vector>
#include "def.h"
#include "common/MeshGraph.h"

class GLDatasetBase
{
public:
  GLDatasetBase(); 
  virtual ~GLDatasetBase();

public:
  void SetDataName(const std::string& dn);
  std::string DataName() const {return _data_name;}

  void SetTimeStep(int timestep, int slot=0);
  int TimeStep(int slot=0) const;
  virtual int NTimeSteps() const {return 0;}

  bool LoadMeshGraph(const std::string& filename);
  bool LoadDefaultMeshGraph();
  void SaveMeshGraph(const std::string& filename);
  void SaveDefaultMeshGraph();

  const MeshGraph* MeshGraph() const {return _mg;}

protected: 
  class MeshGraph *_mg;
  int _timestep, _timestep1; 
  std::string _data_name;
};

#endif
