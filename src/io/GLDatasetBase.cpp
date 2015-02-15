#include "GLDatasetBase.h"

GLDatasetBase::GLDatasetBase() :
  _timestep(-1), _timestep1(-1)
{
}

void GLDatasetBase::SetDataName(const std::string& dn)
{
  _data_name = dn;
}

void GLDatasetBase::SetTimeStep(int t)
{
  _timestep = t;
}

void GLDatasetBase::SetTimeStep1(int t)
{
  _timestep1 = t;
}

bool GLDatasetBase::LoadDefaultMeshGraph()
{
  return LoadMeshGraph(_data_name + ".mg");
}

bool GLDatasetBase::LoadMeshGraph(const std::string& filename)
{
  return _mg.ParseFromFile(filename);
}

void GLDatasetBase::SaveDefaultMeshGraph()
{
  SaveMeshGraph(_data_name + ".mg");
}

void GLDatasetBase::SaveMeshGraph(const std::string& filename)
{
  _mg.SerializeToFile(filename);
}

