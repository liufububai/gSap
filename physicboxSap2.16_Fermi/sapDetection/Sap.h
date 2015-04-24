#ifndef __SAP_H__
#define __SAP_H__

#include "vector_functions.h"
#include "radixsort.h"
//#include "cudpp/cudpp.h"
#include <fstream> // Fu-chang Liu

typedef unsigned int uint;
typedef unsigned int		udword;	
// Particle system class
class Sap
{
public:
    Sap(uint mNbSpheres);
    ~Sap();

	uint nextPow2(uint x);
	void getNumBlocksAndThreads(int whichKernel, int n, int maxBlocks, int maxThreads, int &blocks, int &threads);
	void reduceExt(float* d_odata, float* d_idata, uint nbox, int maxBlocks);
	void Deviation(float* d_odata1, float* d_odata2, float* d_cen, float* d_pos, int* d_lamda, uint* d_thread, int* d_num, float* d_radii, uint nbox, int maxBlocks, float interval);
	void collision(float spacing, float4* d_particleData, int2* pairs, int* pairsEntries, int* NPairs, int* NPEntries);
	void collisionWithSub(float spacing, float4* d_particleData, int2* pairs, int* pairsEntries, int* NPairs, int* NPEntries);

protected: // data
    int mNbSpheres;
	std::ofstream fout;
	bool m_firstTime;
	float interval;

    // CPU data
    //float* m_hPos;              // particle positions
    //float* m_hVel;              // particle velocities
    
	int releNum;
	uint* h_mulpairs;
	uint* h_Entries;
	float* h_cen; 
	float* h_vel;
	//float* ExtList;   
	float* PosList;
    uint* Sorted;
	int maxBlocks;
	uint overlapnum;
	uint asig_num;
	uint avthreadNum;
	uint numpairs;

    // GPU data
    //float* m_dPos;
    //float* m_dVel;
	uint* numPairs;
	uint* numPairEntries;
	int* d_hash;
	int* d_num; // the num of relevant, cross sphere
	float* d_radii; // the maximum radii of cross sphere
	float* sortedCen;
    float* sortedExt;
	float* d_cen;
	float* d_vel;
	float* newVel;
	//float* d_ext;
	float* d_radius;
    float* d_pos;
    uint* d_thread; // record how many threads should be assigned
	uint* id_thread; // record the object number of every thread
    uint* d_Sorted;
    uint* d_pairs;
	uint* d_pairEntries;
	uint* d_mulpairs;
	float* d_mean; // for PCA
	float* d_mean1; // for PCA
	float* d_delta1; // for PCA
	float* d_delta2; // for PCA
	float* d_matrix; // for PCA
	int* d_lamda; // for PCA
	//float* d_U; // for PCA
	//float* d_V; // for PCA

    uint   m_gridSortBits;
    //CUDPPHandle m_sortHandle;
    nvRadixSort::RadixSort *m_sorter;

};

#endif // __SAP_H__