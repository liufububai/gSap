#include "Sap.h"
#include "SapKernel.cuh"

#include <cutil_inline.h>
#include <assert.h>
#include <math.h>
#include <cstdio>
#include <cstdlib>
#define null				0		
#define min(a,b)            (((a) < (b)) ? (a) : (b))

Sap::Sap(uint numSpheres) :
	//m_sorter       (null),
	d_cen           (null),
    //d_ext          (null),
	sortedCen     (null),
	d_radius       (null),
    d_pos           (null),
    d_Sorted       (null),
    d_pairs         (null),
	d_pairEntries (null),
	d_mulpairs    (null),
	d_thread       (null),
	d_mean        (null),
	d_mean1      (null),
	d_delta1       (null),
	d_delta2       (null),
	d_matrix       (null),
	d_lamda       (null),
	//d_U             (null),
	//d_V             (null),
	id_thread      (null),
	//h_mulpairs     (null),
	Sorted          (null),
	d_hash           (null),
	d_num           (null),
	d_radii           (null),
    h_cen           (null),
    h_vel            (null),
	numPairs      (null),
	numPairEntries (null),
	h_mulpairs    (null),
	h_Entries      (null),
	m_firstTime  (true),
	interval         (0),
	fout              ("1.txt")
{
	mNbSpheres = numSpheres;
	asig_num = 256; // average objects number per group, value rank:1,2,4,8,16,32,64, nbox can be divided by it
	avthreadNum = 8;
	overlapnum = 40;//every body 30 pairs.
	numpairs = 60;//every body 30 pairs.
	maxBlocks = 64; // for PCA
	m_gridSortBits = 32;
	bool keysOnly = false;
	m_sorter = new nvRadixSort::RadixSort(mNbSpheres*overlapnum, keysOnly);
	 // Create the CUDPP radix sort
    //CUDPPConfiguration sortConfig;
    //sortConfig.algorithm = CUDPP_SORT_RADIX;
    //sortConfig.datatype = CUDPP_FLOAT;
    //sortConfig.op = CUDPP_ADD;
    //sortConfig.options = CUDPP_OPTION_KEY_VALUE_PAIRS;
    //cudppPlan(&m_sortHandle, sortConfig, mNbSpheres, 1, 0);
	//cutilSafeCall( cudaMalloc( (void**)&d_cen, sizeof(float)*(4*mNbSpheres)) );
    cutilSafeCall( cudaMalloc( (void**)&d_mean, sizeof(float)*(4*maxBlocks)) ); // for PCA
	cutilSafeCall( cudaMalloc( (void**)&d_mean1, sizeof(float)*(4*maxBlocks)) ); // for PCA
	cutilSafeCall( cudaMalloc( (void**)&d_delta1, sizeof(float)*(4*mNbSpheres)) ); // for PCA
	cutilSafeCall( cudaMalloc( (void**)&d_delta2, sizeof(float)*(4*mNbSpheres)) ); // for PCA
	cutilSafeCall( cudaMalloc( (void**)&d_matrix, sizeof(float)*9) ); // for PCA
	cutilSafeCall( cudaMalloc( (void**)&d_lamda, sizeof(int)*3) ); // for PCA
	//cutilSafeCall( cudaMalloc( (void**)&d_U, sizeof(float)*9) ); // for PCA
	//cutilSafeCall( cudaMalloc( (void**)&d_V, sizeof(float)*9) ); // for PCA
	cutilSafeCall( cudaMalloc( (void**)&d_pos, sizeof(float)*(mNbSpheres+1)) );
	cutilSafeCall( cudaMalloc( (void**)&sortedCen, sizeof(float)*(4*mNbSpheres)) );
	cutilSafeCall( cudaMalloc( (void**)&d_hash, sizeof(int)*mNbSpheres) );
	cutilSafeCall( cudaMalloc( (void**)&d_num, sizeof(int)) );
	cutilSafeCall( cudaMalloc( (void**)&d_radii, sizeof(float)) );
	cutilSafeCall( cudaMalloc( (void**)&numPairs, sizeof(uint)) );
	cutilSafeCall( cudaMalloc( (void**)&numPairEntries, sizeof(uint)) );

	cutilSafeCall( cudaMalloc( (void**)&d_thread, sizeof(uint)*(mNbSpheres+1)) );
	cutilSafeCall( cudaMalloc( (void**)&id_thread, sizeof(uint)*((mNbSpheres+1)*avthreadNum)) );
	cutilSafeCall( cudaMalloc( (void**)&d_Sorted, sizeof(uint)*(mNbSpheres+1)) );
	cutilSafeCall( cudaMalloc( (void**) &d_pairs, sizeof(uint)*(mNbSpheres*overlapnum)) );
	cutilSafeCall( cudaMalloc( (void**) &d_pairEntries, sizeof(uint)*(mNbSpheres*numpairs)) );
	cutilSafeCall( cudaMalloc( (void**) &d_mulpairs, sizeof(uint)*(mNbSpheres*asig_num+mNbSpheres*128)) );
}

Sap::~Sap()
{
    mNbSpheres = 0;
	cutilSafeCall(cudaFree(d_mulpairs));
	cutilSafeCall(cudaFree(d_pairs));
	cutilSafeCall(cudaFree(d_pairEntries));
    cutilSafeCall(cudaFree(d_Sorted));
	cutilSafeCall(cudaFree(id_thread));
    cutilSafeCall(cudaFree(d_thread));
	cutilSafeCall(cudaFree(d_mean));
	cutilSafeCall(cudaFree(d_mean1));
    cutilSafeCall(cudaFree(d_delta1));
	cutilSafeCall(cudaFree(d_delta2));
	cutilSafeCall(cudaFree(d_matrix));	
    cutilSafeCall(cudaFree(d_lamda));	
	//cutilSafeCall(cudaFree(d_U));	
	//cutilSafeCall(cudaFree(d_V));	
    cutilSafeCall(cudaFree(d_pos));
    cutilSafeCall(cudaFree(d_cen));
	cutilSafeCall(cudaFree(d_hash));
	cutilSafeCall(cudaFree(d_num));
	cutilSafeCall(cudaFree(d_radii));
	cutilSafeCall(cudaFree(sortedCen));
	delete m_sorter;
	//cudppDestroyPlan(m_sortHandle);
}

uint
Sap::nextPow2(uint x)
{
	--x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

void Sap::getNumBlocksAndThreads(int whichKernel, int n, int maxBlocks, int maxThreads, int &blocks, int &threads)
{
        
	//printf("n %-5.6f ", n);
    if (whichKernel < 3)
    {
        threads = (n < maxThreads) ? nextPow2(n) : maxThreads;
        blocks = (n + threads - 1) / threads;
    }
    else
    {
        threads = (n < maxThreads*2) ? nextPow2((n + 1)/ 2) : maxThreads;
        blocks = (n + (threads * 2 - 1)) / (threads * 2);
    }
        

    if (whichKernel == 6)
        blocks = min(maxBlocks, blocks);
}

void Sap::reduceExt(float* d_odata, float* d_idata, uint nbox, int maxBlocks)
{
	   int size = nbox;
	   int maxThreads = 128;
	   int whichKernel = 6; 
	   //int maxBlocks = 64;
	   int numBlocks = 0;
	   int numThreads = 0;
	   //float gpu_result[4];
       //gpu_result[0] = 0;
       //gpu_result[1] = 0;
	   //gpu_result[2] = 0;

	   int cpuFinalThreshold = 1;
	   getNumBlocksAndThreads(whichKernel, size, maxBlocks, maxThreads, numBlocks, numThreads);
	   
	   reduce(size, numThreads, numBlocks, whichKernel, d_idata, d_odata);
	// sum partial block sums on GPU
		int s=numBlocks;
		int kernel = (whichKernel == 6) ? 5 : whichKernel;
		while(s > cpuFinalThreshold) 
		{
			int threads = 0, blocks = 0; 
			getNumBlocksAndThreads(kernel, s, maxBlocks, maxThreads, blocks, threads);
	        
			reduce(s, threads, blocks, kernel, d_odata, d_odata);
	        
			if (kernel < 3)
				s = (s + threads - 1) / threads;
			else
				s = (s + (threads*2-1)) / (threads*2);
		}

		// copy result from device to host
		//cutilSafeCall( cudaMemcpy(&gpu_result, d_odata, sizeof(float)*4, cudaMemcpyDeviceToHost) ); // cost over 3 ms

		//printf("GPU_mean %-5.6f  %-5.6f  %-5.6f\n", gpu_result[0]/size, gpu_result[1]/size, gpu_result[2]/size);//
}

void Sap::Deviation(float* d_odata1, float* d_odata2, float* d_cen, float* d_pos, int* d_lamda, uint* d_thread, int* d_num, float* d_radii, uint nbox, int maxBlocks, float interval)
{
	 //float gpu_result1[3], gpu_result2[3];

     reduceExt(d_odata1, d_cen, nbox, maxBlocks);
	 cutilSafeCall( cudaMemcpy(d_matrix, d_odata1, sizeof(float)*3, cudaMemcpyDeviceToDevice) );
	 //cutilSafeCall( cudaMemcpy(gpu_result1, d_matrix, sizeof(float)*3, cudaMemcpyDeviceToHost) );
	 //printf("GPU_mean %-5.6f  %-5.6f  %-5.6f\n", gpu_result1[0]/nbox, gpu_result1[1]/nbox, gpu_result1[2]/nbox);
     calDelta(d_odata1, d_cen, d_delta1, d_delta2, nbox);
	 reduceExt(d_odata1, d_delta1, nbox, maxBlocks);
	 reduceExt(d_odata2, d_delta2, nbox, maxBlocks);
	// copy result from device to host
	//cutilSafeCall( cudaMemcpy(gpu_result1, d_odata1, sizeof(float)*3, cudaMemcpyDeviceToHost) ); // cost over 3 ms
	//cutilSafeCall( cudaMemcpy(gpu_result2, d_odata2, sizeof(float)*3, cudaMemcpyDeviceToHost) ); // cost over 3 ms
	//printf("GPU_deviation1 %-5.6f  %-5.6f  %-5.6f\n", gpu_result1[0], gpu_result1[1], gpu_result1[2]);
	//printf("GPU_deviation2 %-5.6f  %-5.6f  %-5.6f\n", gpu_result2[0], gpu_result2[1], gpu_result2[2]);
    //releNum = optimalAxis(d_odata1, d_odata2, d_cen, d_pos, d_Sorted, d_matrix, d_lamda, d_thread, d_num, d_radii, nbox, interval);
	// ProjectOnAxis(d_odata1, d_odata2, d_cen, d_pos, d_Sorted, d_matrix, d_lamda, nbox, interval);//without subdivision
	//ElongOnAxis(d_odata1, d_odata2, d_cen, d_pos, d_Sorted, d_matrix, d_lamda, nbox, interval);
	ElongOnAxisMultiple(d_odata1, d_odata2, d_cen, d_pos, d_Sorted, d_matrix, d_lamda, nbox, interval);
	//cutilSafeCall( cudaMemcpy(gpu_result1, d_lamda, sizeof(int)*3, cudaMemcpyDeviceToHost) );
	//printf("GPU_lamda %-5d %-5d  %-5d\n", gpu_result1[0], gpu_result1[1], gpu_result1[2]);
}

void Sap::collision(float spacing, float4* d_particleData, int2* pairs, int* pairsEntries, int* NPairs, int* NPEntries)
{
	cudaEvent_t start_event, stop_event;
    cutilSafeCall( cudaEventCreate(&start_event) );
    cutilSafeCall( cudaEventCreate(&stop_event) );
	cutilSafeCall( cudaEventRecord(start_event, 0) );

	uint pairsNum1 = 0;
	uint pairsNum2 = 0;
	uint pairsNum = 0;
	uint nPairs = 0;
	uint nPairEntries = 0;
	float4 maxE, minE;
	float t_interval;

	if (m_firstTime)
	{
		t_interval = spacing;
		m_firstTime = false;
	}
	else
		t_interval = interval;

	Deviation(d_mean, d_mean1, (float*)d_particleData, d_pos, d_lamda, d_thread, d_num, d_radii, mNbSpheres, maxBlocks, t_interval);
	m_sorter->sort((float*)d_pos, d_Sorted, mNbSpheres, m_gridSortBits, true);
    //cudppSort(m_sortHandle, d_pos, d_Sorted, m_gridSortBits, mNbSpheres);
	pairsNum1 = sweepPrunSphereSubElong((float*)d_particleData, d_pos, d_Sorted,  d_pairs, d_thread, id_thread, d_mulpairs, mNbSpheres, releNum, overlapnum, 512, avthreadNum, sortedCen, d_hash, d_mean, d_lamda, t_interval, d_matrix);
    
	CUDA_SAFE_CALL( cudaMemcpy(&maxE, &(sortedCen[4*(mNbSpheres-1)]), sizeof(float4), cudaMemcpyDeviceToHost) );
	CUDA_SAFE_CALL( cudaMemcpy(&minE, &(sortedCen[0]), sizeof(float4), cudaMemcpyDeviceToHost) );
	interval = maxE.x-minE.x > maxE.y-minE.y ? maxE.x-minE.x : maxE.y-minE.y;
	interval = interval > maxE.z-minE.z ? interval : maxE.z-minE.z;
	m_firstTime = false;
	//printf("maxE.x y z  minE.x y z  interval  %-5.3f  %-5.3f  %-5.3f  %-5.3f  %-5.3f  %-5.3f  %-5.3f \n ", maxE.x, maxE.y, maxE.z, minE.x, minE.y, minE.z, interval );

	releNum = optimalAxisMultiple(d_mean, d_mean1, (float*)d_particleData, d_pos, d_Sorted, d_matrix, d_lamda, d_thread, d_num, d_radii, mNbSpheres, t_interval);
	//releNum =  optimalAxis(d_mean, d_mean1, (float*)d_particleData, d_pos, d_Sorted, d_matrix, d_lamda, d_thread, d_num, d_radii, mNbSpheres, interval);
	//printf("releNum%-5d\n ", releNum );
	if(releNum > 0 && releNum < mNbSpheres)
	{
		m_sorter->sort((float*)d_pos, d_Sorted, releNum, m_gridSortBits, true);// record time
		//cudppSort(m_sortHandle, d_pos, d_Sorted, m_gridSortBits, mNbSpheres);
		pairsNum2 = sweepPrunSphereSubCross((float*)d_particleData, d_pos, d_Sorted,  d_pairs, d_thread, id_thread, d_mulpairs, mNbSpheres, releNum, pairsNum1, overlapnum, 128, avthreadNum, sortedCen, d_hash, d_mean, d_lamda, t_interval, d_matrix);
	}
	pairsNum =pairsNum1 + pairsNum2;
   
	/*h_cen = new float[mNbSpheres*4];
	cutilSafeCall( cudaMemcpy(h_cen, (float*)d_particleData, sizeof(float)*mNbSpheres*4, cudaMemcpyDeviceToHost) );
	if(pairsNum1 > 70)
	for(uint i=0;i<mNbSpheres;i++) // for validation
		 {
			 fout<<"i :"<<i<<'\n'
				<<h_cen[i*4]<<'\n'
				 <<h_cen[i*4+1]<<'\n'
				 <<h_cen[i*4+2]<<'\n'
				 <<h_cen[i*4+3]<<"\n\n";
		 }*/

	if(pairsNum > 0)
	{
		copyPairs(d_pairs, d_mulpairs, pairsNum1, pairsNum2, mNbSpheres, overlapnum); 
		setSortIndex(d_pairs, 2*pairsNum);
		m_sorter->sort(d_mulpairs, d_pairs, 2*pairsNum, 23);
		//cudppSort(m_sortHandle, d_mulpairs, d_pairs, 32, 2*pairsNum);
		movePairs(d_mulpairs, pairsNum, 0, 2);
		orderPairs(d_pairs, d_mulpairs+2*pairsNum, d_mulpairs, 2*pairsNum);
		setSortIndex(d_pairs, 2*pairsNum);
		m_sorter->sort(d_mulpairs, d_pairs, 2*pairsNum, 23);
		//cudppSort(m_sortHandle, d_mulpairs, d_pairs, 32, 2*pairsNum);
		orderPairs(d_pairs, d_mulpairs+4*pairsNum, d_mulpairs+2*pairsNum, 2*pairsNum);

		/*h_mulpairs = new uint[mNbSpheres*overlapnum];
		cutilSafeCall( cudaMemcpy(h_mulpairs, d_mulpairs, sizeof(uint)*mNbSpheres*overlapnum, cudaMemcpyDeviceToHost) );
		for(uint i=0;i<2*pairsNum;i++) // for validation
		{
			fout<<"("<<h_mulpairs[i]<<"\t"<<h_mulpairs[i+2*pairsNum]<<")\n";
		}
		fout<<"overlap pairs "<<pairsNum<<"\n";
		fout<<"-------------------------------------------------------------\n";*/

		cullRepetitivePairs(d_mulpairs, d_pairs, d_pairEntries, d_thread, 2*pairsNum, numPairs, numPairEntries); 
		//nPairEntries = extrPairsEntries(d_mulpairs, d_pairs, d_pairEntries, d_thread, 2*pairsNum); // if no repetitive pairs, directly extract pairsEntries
	
	cutilSafeCall(cudaMemset(d_mulpairs, 0, 6*pairsNum*sizeof(uint))); 
	}//if(pairsNum > 0)

	cutilSafeCall( cudaEventRecord(stop_event, 0) );
	cutilSafeCall( cudaEventSynchronize(stop_event) );

	float time = 0;
	cutilSafeCall( cudaEventElapsedTime(&time, start_event, stop_event));

	CUDA_SAFE_CALL( cudaMemcpy(&nPairs, numPairs, sizeof(uint), cudaMemcpyDeviceToHost) );
	CUDA_SAFE_CALL( cudaMemcpy(&nPairEntries, numPairEntries, sizeof(uint), cudaMemcpyDeviceToHost) );
	//printf("collison detect time:: %-5.3f\n", time);
    //printf("collison pairs num:: %-5d\n", pairsNum);
	//printf("numPairs:: %-5d\n", nPairs);
    //printf("numPairEntries:: %-5d\n", nPairEntries);
	/*h_mulpairs = new uint[nPairs*2];
	h_Entries = new uint[nPairEntries*2];
	cutilSafeCall( cudaMemcpy(h_mulpairs, d_pairs, sizeof(uint)*nPairs*2, cudaMemcpyDeviceToHost) );
	cutilSafeCall( cudaMemcpy(h_Entries , d_pairEntries, sizeof(uint)*nPairEntries*2, cudaMemcpyDeviceToHost) );
	for(uint i=0;i<nPairs;i++) // for validation
	{
		fout<<"("<<h_mulpairs[2*i]<<"\t"<<h_mulpairs[2*i+1]<<")\n";
	}
	fout<<"overlap pairs "<<nPairs<<"\n";
	fout<<"-------------------------------------------------------------\n";
	for(uint i=0;i<nPairEntries;i++) // for validation
		fout<<h_Entries[i]<<"\n";
	fout<<"pairsEntries "<<nPairEntries<<"\n";*/

	//nPairs = 2*pairsNum;
	//if(nPairs < 5000)
	{
		CUDA_SAFE_CALL( cudaMemcpy(pairs, d_pairs, sizeof(int)*2*nPairs, cudaMemcpyDeviceToDevice) );
		CUDA_SAFE_CALL( cudaMemcpy(pairsEntries, d_pairEntries, sizeof(int)*nPairEntries, cudaMemcpyDeviceToDevice) );
		*NPairs = nPairs;
		*NPEntries = nPairEntries;
	}

	cutilSafeCall(cudaEventDestroy(start_event));
	cutilSafeCall(cudaEventDestroy(stop_event));
	//fout<<"collison detect time::   \t"<< time<<"\t\t" <<"collison pairs::   \t"<<nPairs<<"\n";
}

void Sap::collisionWithSub(float spacing, float4* d_particleData, int2* pairs, int* pairsEntries, int* NPairs, int* NPEntries)
{
	/*cudaEvent_t start_event, stop_event;
    cutilSafeCall( cudaEventCreate(&start_event) );
    cutilSafeCall( cudaEventCreate(&stop_event) );
	cutilSafeCall( cudaEventRecord(start_event, 0) );

	uint pairsNum1 = 0;
	uint pairsNum2 = 0;
	uint pairsNum = 0;
	uint nPairs = 0;
	uint nPairEntries = 0;


	float interval = spacing; //???
	Deviation(d_mean, d_mean1, (float*)d_particleData, d_pos, d_lamda, d_thread, d_num, d_radii, mNbSpheres, maxBlocks, interval);
	m_sorter->sort((float*)d_pos, d_Sorted, mNbSpheres, m_gridSortBits, true);
	pairsNum = sweepPrunSphereSubElong((float*)d_particleData, d_pos, d_Sorted,  d_pairs, d_thread, id_thread, d_mulpairs, mNbSpheres, releNum, overlapnum, asig_num, avthreadNum, sortedCen, d_hash, d_mean, d_lamda, interval, d_matrix);

	if(pairsNum > 0)
	{
		copyPairsWithoutSub(d_pairs, d_mulpairs, pairsNum, 0, mNbSpheres, overlapnum); 
		setSortIndex(d_pairs, 2*pairsNum);
		m_sorter->sort(d_mulpairs, d_pairs, 2*pairsNum, 23);
        orderPairs(d_pairs, d_mulpairs+2*pairsNum, d_mulpairs+4*pairsNum, 2*pairsNum);

		//cullRepetitivePairs(d_mulpairs, d_pairs, d_pairEntries, d_thread, 2*pairsNum, numPairs, numPairEntries); 
		nPairEntries = extrPairsEntries(d_mulpairs, d_pairs, d_pairEntries, d_thread, 2*pairsNum); // if no repetitive pairs, directly extract pairsEntries
		/*h_mulpairs = new uint[pairsNum*4];
		h_Entries = new uint[nPairEntries*2];
		cutilSafeCall( cudaMemcpy(h_mulpairs, d_pairs, sizeof(uint)*pairsNum*4, cudaMemcpyDeviceToHost) );
		cutilSafeCall( cudaMemcpy(h_Entries , d_pairEntries, sizeof(uint)*nPairEntries*2, cudaMemcpyDeviceToHost) );
		h_cen = new float[mNbSpheres*4];
		cutilSafeCall( cudaMemcpy(h_cen, (float*)d_particleData, sizeof(float)*mNbSpheres*4, cudaMemcpyDeviceToHost) );
		for(uint i=0;i<mNbSpheres;i++) // for validation
		 {
			 if(h_mulpairs[i] == 1118	 && h_mulpairs[i+2*pairsNum] == 540)
			 fout<<"i :"<<i<<'\n'
				<<h_cen[i*4]<<'\n'
				 <<h_cen[i*4+1]<<'\n'
				 <<h_cen[i*4+2]<<'\n'
				 <<h_cen[i*4+3]<<"\n\n";
		 }*/

		/*for(uint i=0;i<2*pairsNum;i++) // for validation
		{
			fout<<"("<<h_mulpairs[i]<<"\t"<<h_mulpairs[i+2*pairsNum]<<")\n";
		}
		fout<<h_cen[1118	*4]<<h_cen[540*4]<<'\n'
				  <<h_cen[1118	*4+1]<<h_cen[540*4+1]<<'\n'
				  <<h_cen[1118	*4+2]<<h_cen[540*4+2]<<'\n'
				  <<h_cen[1118	*4+3]<<h_cen[540*4+3]<<"\n"
		          <<"overlap pairs "<<2*pairsNum<<"\n";
	    float x = h_cen[1118	*4] - h_cen[540	*4];
		float y = h_cen[1118	*4+1] - h_cen[540	*4+1];
		float z = h_cen[1118	*4+2] - h_cen[540	*4+2];
		float r = h_cen[1118	*4+3] + h_cen[540	*4+3];
		float collide = x*x + y*y + z*z - r*r;
		fout<<"----------------1118--collide----540------------"<<collide<<"\n";
		for(uint i=0;i<nPairEntries;i++) // for validation
			fout<<h_Entries[i]<<"\n";
		fout<<"pairsEntries "<<nPairEntries<<"\n";*/
	
	/*cutilSafeCall(cudaMemset(d_mulpairs, 0, 6*pairsNum*sizeof(uint))); 
	}//if(pairsNum > 0)

	cutilSafeCall( cudaEventRecord(stop_event, 0) );
	cutilSafeCall( cudaEventSynchronize(stop_event) );

	float time = 0;
	cutilSafeCall( cudaEventElapsedTime(&time, start_event, stop_event));

	//CUDA_SAFE_CALL( cudaMemcpy(&nPairs, numPairs, sizeof(uint), cudaMemcpyDeviceToHost) );
	//CUDA_SAFE_CALL( cudaMemcpy(&nPairEntries, numPairEntries, sizeof(uint), cudaMemcpyDeviceToHost) );
	//printf("collison detect time:: %-5.3f\n", time);
    //printf("collison pairs num:: %-5d\n", pairsNum);
	//printf("numPairs:: %-5d\n", nPairs);
    //printf("numPairEntries:: %-5d\n", nPairEntries);
	nPairs = 2*pairsNum;
	//if(nPairs < 5000)
	{
		CUDA_SAFE_CALL( cudaMemcpy(pairs, (int2*)d_pairs, sizeof(int2)*nPairs, cudaMemcpyDeviceToDevice) );
		CUDA_SAFE_CALL( cudaMemcpy(pairsEntries, d_pairEntries, sizeof(int)*nPairEntries, cudaMemcpyDeviceToDevice) );
		*NPairs = nPairs;
		*NPEntries = nPairEntries;
	}

	cutilSafeCall(cudaEventDestroy(start_event));
	cutilSafeCall(cudaEventDestroy(stop_event));
	fout<<"collison detect time::   \t"<< time<<"\t\t" <<"collison pairs time::   \t"<<pairsNum<<"\n";*/
}