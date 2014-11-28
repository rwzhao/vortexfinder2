#include <assert.h>
#include <list>
#include "extractor.h"
#include "utils.h"

VortexExtractor::VortexExtractor(const Parallel::Communicator &comm)
  : ParallelObject(comm), 
    _verbose(0), 
    _mesh(NULL), 
    _exio(NULL), 
    _eqsys(NULL), 
    _tsys(NULL),
    _gauge(false)
{
}

VortexExtractor::~VortexExtractor()
{
  if (_eqsys) delete _eqsys; 
  if (_exio) delete _exio; 
  if (_mesh) delete _mesh; 
}

void VortexExtractor::SetVerbose(int level)
{
  _verbose = level; 
}

void VortexExtractor::SetMagneticField(const double B[3])
{
  memcpy(_B, B, sizeof(double)*3); 
}

void VortexExtractor::SetKex(double Kex)
{
  _Kex = Kex; 
}

void VortexExtractor::SetGaugeTransformation(bool g)
{
  _gauge = g; 
}

void VortexExtractor::LoadData(const std::string& filename)
{
  /// mesh
  _mesh = new Mesh(comm()); 
  _exio = new ExodusII_IO(*_mesh);
  _exio->read(filename);
  _mesh->allow_renumbering(false); 
  _mesh->prepare_for_use();

  if (Verbose())
    _mesh->print_info(); 

  /// equation systems
  _eqsys = new EquationSystems(*_mesh); 
  
  _tsys = &(_eqsys->add_system<NonlinearImplicitSystem>("GLsys"));
  _u_var = _tsys->add_variable("u", FIRST, LAGRANGE);
  _v_var = _tsys->add_variable("v", FIRST, LAGRANGE); 

  _eqsys->init(); 
  
  if (Verbose())
    _eqsys->print_info();
}

void VortexExtractor::LoadTimestep(int timestep)
{
  assert(_exio != NULL); 

  _timestep = timestep;

  if (Verbose())
    fprintf(stderr, "copying nodal solution... timestep=%d\n", timestep); 

  /// copy nodal data
  _exio->copy_nodal_solution(*_tsys, "u", "u", timestep); 
  _exio->copy_nodal_solution(*_tsys, "v", "v", timestep);
  
  if (Verbose())
    fprintf(stderr, "nodal solution copied, timestep=%d\n", timestep); 
}

void VortexExtractor::Extract()
{
  if (Verbose())
    fprintf(stderr, "extracting singularities on mesh faces...\n"); 
  
  const DofMap &dof_map  = _tsys->get_dof_map();  
  MeshBase::const_element_iterator it = _mesh->active_local_elements_begin(); 
  const MeshBase::const_element_iterator end = _mesh->active_local_elements_end(); 
 
  std::vector<float> zeros; 

  for (; it!=end; it++) {
    const Elem *elem = *it;
    PuncturedElem<double> punctured_elem; 
    
    for (int face=0; face<elem->n_sides(); face++) {
      AutoPtr<Elem> side = elem->side(face); 
      
      std::vector<dof_id_type> u_di, v_di; 
      dof_map.dof_indices(side.get(), u_di, _u_var);
      dof_map.dof_indices(side.get(), v_di, _v_var);
     
      // could use solution->get()
      double u[3] = {(*_tsys->solution)(u_di[0]), (*_tsys->solution)(u_di[1]), (*_tsys->solution)(u_di[2])}, 
             v[3] = {(*_tsys->solution)(v_di[0]), (*_tsys->solution)(v_di[1]), (*_tsys->solution)(v_di[2])};

      Node *nodes[3] = {side->get_node(0), side->get_node(1), side->get_node(2)};
      double X0[3] = {nodes[0]->slice(0), nodes[0]->slice(1), nodes[0]->slice(2)}, 
             X1[3] = {nodes[1]->slice(0), nodes[1]->slice(1), nodes[1]->slice(2)}, 
             X2[3] = {nodes[2]->slice(0), nodes[2]->slice(1), nodes[2]->slice(2)};
      double rho[3] = {sqrt(u[0]*u[0]+v[0]*v[0]), sqrt(u[1]*u[1]+v[1]*v[1]), sqrt(u[2]*u[2]+v[2]*v[2])}, 
             phi[3] = {atan2(v[0], u[0]), atan2(v[1], u[1]), atan2(v[2], u[2])}; 

      // check phase shift
      double flux = 0.f; // TODO: need to compute the flux correctly
      double delta[3];

      if (_gauge) {
        delta[0] = phi[1] - phi[0] - gauge_transformation(X0, X1, _Kex, _B); 
        delta[1] = phi[2] - phi[1] - gauge_transformation(X1, X2, _Kex, _B); 
        delta[2] = phi[0] - phi[2] - gauge_transformation(X2, X0, _Kex, _B); 
      } else {
        delta[0] = phi[1] - phi[0]; 
        delta[1] = phi[2] - phi[1]; 
        delta[2] = phi[0] - phi[2]; 
      }

      double delta1[3];  
      double phase_shift = 0.f; 
      for (int k=0; k<3; k++) {
        delta1[k] = mod2pi(delta[k] + M_PI) - M_PI; 
        phase_shift += delta1[k]; 
      }
      phase_shift += flux; 
      double critera = phase_shift / (2*M_PI);
      if (fabs(critera)<0.5f) continue; // not punctured
     
      // update bits
      int chirality = lround(critera);
      punctured_elem.elem = elem; 
      punctured_elem.SetChirality(face, chirality);
      punctured_elem.SetPuncturedFace(face);

      if (_gauge) {
        phi[1] = phi[0] + delta1[0]; 
        phi[2] = phi[1] + delta1[1];
        u[1] = rho[1] * cos(phi[1]);
        v[1] = rho[1] * sin(phi[1]); 
        u[2] = rho[2] * cos(phi[2]); 
        v[2] = rho[2] * sin(phi[2]);
      }

      double pos[3]; 
      bool succ = find_zero_triangle(u, v, X0, X1, X2, pos); 
      if (succ) {
        punctured_elem.SetPuncturedPoint(face, pos); 

        zeros.push_back(pos[0]); 
        zeros.push_back(pos[1]); 
        zeros.push_back(pos[2]);
      } else {
        fprintf(stderr, "WARNING: punctured but singularities not found\n"); 
      }
    }
  
    if (punctured_elem.Valid()) {
      _punctured_elems.insert(std::make_pair<const Elem*, PuncturedElem<double> >(elem, punctured_elem));  
      fprintf(stderr, "elem_id=%d, bits=%s\n", 
          elem->id(), punctured_elem.bits.to_string().c_str()); 
    }
  }
  
  int npts = zeros.size()/3; 
  fprintf(stderr, "total number of singularities: %d\n", npts); 
  FILE *fp = fopen("out", "wb");
  fwrite(&npts, sizeof(int), 1, fp); 
  fwrite(zeros.data(), sizeof(float), zeros.size(), fp); 
  fclose(fp); 
}

void VortexExtractor::Trace(VortexObject& vortex, punctured_elem_iterator it, int direction)
{
  _traced_punctured_elems.push_back(it);
  const PuncturedElem<double>& punctured_elem = it->second;

  for (int face=0; face<4; face++) {
    if (punctured_elem.Chirality(face) == direction) {
     const Elem *next_elem = punctured_elem.elem->neighbor(face);
     if (next_elem) {
        punctured_elem_iterator it1 = _punctured_elems.find(next_elem); 
        Trace(vortex, it1, direction);
      }
    }
  }
}

void VortexExtractor::Trace()
{
#if 0
  while (!_punctured_elems.empty()) {
    std::map<Elem*, PuncturedElem<double> >::iterator it = _punctured_elems.begin();

    dof_id_type id = it->first;
    // forward trace 
    
    // backward trace
  }
#endif
}
