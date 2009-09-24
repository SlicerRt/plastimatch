/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */


#define _CRT_SECURE_NO_DEPRECATE
#define READ_PFM
#define WRITE_BLOCK (1024*1024)

/****************************************************\
* Uncomment the line below to enable verbose output. *
* Enabling this should not nerf performance.         *
\****************************************************/
//#define VERBOSE

/**********************************************************\
* Uncomment the line below to enable detailed performance  *
* reporting.  This measurement alters the system, however, *
* resulting in significantly slower kernel execution.      *
\**********************************************************/
#define TIME_KERNEL

/*****************
*  C   #includes *
*****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*****************
* CUDA #includes *
*****************/
#include <cuda.h>

/*****************
* FDK  #includes *
*****************/
#include "fdk.h"
#include "fdk_opts_ext.h"
#include "fdk_cuda.h"
#include "volume.h"
#include "readmha_ext.h"
#include "mathutil.h"
#include "C:\AAAfiles\Anthony\plastimatch\plastimatch\trunk\fdk_utils2.h"
/*********************
* High Res Win Timer *
*********************/
#include <time.h>
#include <windows.h>
#include <winbase.h>
void free_cb_image (CB_Image* cbi);


// P R O T O T Y P E S ////////////////////////////////////////////////////
CB_Image* get_image (MGHCBCT_Options_ext* options, int image_num);
void checkCUDAError(const char *msg);

__global__ void kernel_fdk (float *dev_vol, int2 img_dim, float2 ic, float3 nrm, float sad, float scale, float3 vol_offset, int3 vol_dim, float3 vol_pix_spacing, unsigned int Blocks_Y, float invBlocks_Y);
///////////////////////////////////////////////////////////////////////////



// T E X T U R E S ////////////////////////////////////////////////////////
texture<float, 1, cudaReadModeElementType> tex_img;
texture<float, 1, cudaReadModeElementType> tex_matrix;
///////////////////////////////////////////////////////////////////////////



//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_( S T A R T )_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
__global__
void kernel_fdk (float *dev_vol, int2 img_dim, float2 ic, float3 nrm, float sad, float scale, float3 vol_offset, int3 vol_dim, float3 vol_pix_spacing, unsigned int Blocks_Y, float invBlocks_Y)
{
	// CUDA 2.0 does not allow for a 3D grid, which severely
	// limits the manipulation of large 3D arrays of data.  The
	// following code is a hack to bypass this implementation
	// limitation.
	unsigned int blockIdx_z = __float2uint_rd(blockIdx.y * invBlocks_Y);
	unsigned int blockIdx_y = blockIdx.y - __umul24(blockIdx_z, Blocks_Y);
	unsigned int i = __umul24(blockIdx.x, blockDim.x) + threadIdx.x;
	unsigned int j = __umul24(blockIdx_y, blockDim.y) + threadIdx.y;
	unsigned int k = __umul24(blockIdx_z, blockDim.z) + threadIdx.z;

	if( i >= vol_dim.x || j >= vol_dim.y || k >= vol_dim.z )
		return; 
/*
	// EXPERIMENTAL //////////////////////////////////////////////
	long int one_block = blockDim.x * blockDim.y * blockDim.z;
	long int offset_x  = blockIdx.x * (one_block);
	long long int offset_y  = blockIdx.y * (one_block * gridDim.x);
	long int block_index = threadIdx.x + (blockDim.x * threadIdx.y) + (blockDim.x * blockDim.y * threadIdx.z);
	long int vol_idx = offset_x + offset_y + block_index;
	//////////////////////////////////////////////////////////////
*/
	// Index row major into the volume
	long int vol_idx = i + ( j*(vol_dim.x) ) + ( k*(vol_dim.x)*(vol_dim.y) );

	float3 vp;
	float3 ip;
	float  s;
	float voxel_data;

	float4 matrix_x;
	float4 matrix_y;
	float4 matrix_z;

	// offset volume coords
	vp.x = vol_offset.x + i * vol_pix_spacing.x;	// Compiler should combine into 1 FMAD.
	vp.y = vol_offset.y + j * vol_pix_spacing.y;	// Compiler should combine into 1 FMAD.
	vp.z = vol_offset.z + k * vol_pix_spacing.z;	// Compiler should combine into 1 FMAD.

	// matrix multiply
	ip.x = tex1Dfetch(tex_matrix, 0)*vp.x + tex1Dfetch(tex_matrix, 1)*vp.y + tex1Dfetch(tex_matrix, 2)*vp.z + tex1Dfetch(tex_matrix, 3);
	ip.y = tex1Dfetch(tex_matrix, 4)*vp.x + tex1Dfetch(tex_matrix, 5)*vp.y + tex1Dfetch(tex_matrix, 6)*vp.z + tex1Dfetch(tex_matrix, 7);
	ip.z = tex1Dfetch(tex_matrix, 8)*vp.x + tex1Dfetch(tex_matrix, 9)*vp.y + tex1Dfetch(tex_matrix, 10)*vp.z + tex1Dfetch(tex_matrix, 11);

	// Change coordinate systems
	ip.x = ic.x + ip.x / ip.z;
	ip.y = ic.y + ip.y / ip.z;

	// Get pixel from 2D image
	ip.x = __float2int_rd(ip.x);
	ip.y = __float2int_rd(ip.y);
	voxel_data = tex1Dfetch(tex_img, ip.x*img_dim.x + ip.y);

	// Dot product
	s = nrm.x*vp.x + nrm.y*vp.y + nrm.z*vp.z;

	// Conebeam weighting factor
	s = sad - s;
	s = (sad * sad) / (s * s);

	// Place it into the volume
	dev_vol[vol_idx] += scale * s * voxel_data;
}
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-( E N D )-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_



///////////////////////////////////////////////////////////////////////////
// FUNCTION: CUDA_reconstruct_conebeam() //////////////////////////////////
extern "C"
int CUDA_reconstruct_conebeam (Volume *vol, MGHCBCT_Options_ext *options)
{
	// Thead Block Dimensions
	int tBlock_x = 16;
	int tBlock_y = 4;
	int tBlock_z = 4;

	// Each element in the volume (each voxel) gets 1 thread
	int blocksInX = (vol->dim[0]+tBlock_x-1)/tBlock_x;
	int blocksInY = (vol->dim[1]+tBlock_y-1)/tBlock_y;
	int blocksInZ = (vol->dim[2]+tBlock_z-1)/tBlock_z;
	dim3 dimGrid  = dim3(blocksInX, blocksInY*blocksInZ);
	dim3 dimBlock = dim3(tBlock_x, tBlock_y, tBlock_z);

	// Size of volume Malloc
	int vol_size_malloc = (vol->dim[0]*vol->dim[1]*vol->dim[2])*sizeof(float);

	// Structure for passing arugments to kernel: (See fdk_cuda.h)
	kernel_args_fdk *kargs;
	kargs = (kernel_args_fdk *) malloc(sizeof(kernel_args_fdk));

	CB_Image* cbi;
	int image_num;
	int i;

	// CUDA device pointers
	float *dev_vol;	            // Holds voxels on device
	float *dev_img;	            // Holds image pixels on device
	float *dev_matrix;
	kernel_args_fdk *dev_kargs; // Holds kernel parameters
	cudaMalloc( (void**)&dev_matrix, 12*sizeof(float) );
	cudaMalloc( (void**)&dev_kargs, sizeof(kernel_args_fdk) );

	// Calculate the scale
	image_num = 1 + (options->last_img - options->first_img) / options->skip_img;
	float scale = (float) (sqrt(3.0) / (double) image_num);
	scale = scale * options->scale;

	// Load static kernel arguments
	kargs->scale = scale;
	kargs->vol_offset.x = vol->offset[0];
	kargs->vol_offset.y = vol->offset[1];
	kargs->vol_offset.z = vol->offset[2];
	kargs->vol_dim.x = vol->dim[0];
	kargs->vol_dim.y = vol->dim[1];
	kargs->vol_dim.z = vol->dim[2];
	kargs->vol_pix_spacing.x = vol->pix_spacing[0];
	kargs->vol_pix_spacing.y = vol->pix_spacing[1];
	kargs->vol_pix_spacing.z = vol->pix_spacing[2];


		////// TIMING CODE //////////////////////
		// Initialize Windows HighRes Timer
		int count,a;
		float kernel_total, io_total;
		LARGE_INTEGER ticksPerSecond;
		LARGE_INTEGER tick;   // A point in time
		LARGE_INTEGER start_ticks_kernel, end_ticks_kernel, cputime;   
		LARGE_INTEGER start_ticks_io, end_ticks_io;   
		LARGE_INTEGER start_ticks_total, end_ticks_total;   

		#if defined (VERBOSE)
			printf("\n\nInitializing High Resolution Timers\n");
		#endif
		io_total=0;
		kernel_total=0;

		// get the high resolution counter's accuracy
		if (!QueryPerformanceFrequency(&ticksPerSecond))
			printf("QueryPerformance not present!");

		#if defined (VERBOSE)
			printf ("\tFreq Test:   %I64Ld ticks/sec\n",ticksPerSecond    );
		#endif

		// Test: Get current time.
		if (!QueryPerformanceCounter(&tick))
			printf("no go counter not installed");  

		#if defined (VERBOSE)
			printf ("\tTestpoint:   %I64Ld  ticks\n",tick);
		#endif
		/////////////////////////////////////////
	

	#if defined (VERBOSE)
		// First, we need to allocate memory on the host device
		// for the 3D volume of voxels that will hold our reconstruction.
		printf("========================================\n");
		printf("Allocating %dMB of video memory...", vol_size_malloc/1000000);
	#endif

	cudaMalloc( (void**)&dev_vol, vol_size_malloc);
	cudaMemset( (void *) dev_vol, 0, vol_size_malloc);	
	checkCUDAError("Unable to allocate data volume");

	#if defined (VERBOSE)
		printf(" done.\n\n");

		// State the kernel execution parameters
		printf("kernel parameters:\n dimGrid: %u, %u (Logical: %u, %u, %u)\n dimBlock: %u, %u, %u\n", dimGrid.x, dimGrid.y, dimGrid.x, blocksInY, blocksInZ, dimBlock.x, dimBlock.y, dimBlock.z);
		printf("%u voxels in volume\n", vol->npix);
		printf("%u projections to process\n", 1+(options->last_img - options->first_img) / options->skip_img);
		printf("%u Total Operations\n", vol->npix * (1+(options->last_img - options->first_img) / options->skip_img));
		printf("========================================\n\n");

		// Start working
		printf("Processing...");
	#endif

	
	
	// This is just to retrieve the 2D image dimensions
	int fimg=options->first_img;
		do{
		cbi = get_image(options, fimg);
			fimg++;
			}
			while(cbi==NULL);
		
	cudaMalloc( (void**)&dev_img, cbi->dim[0]*cbi->dim[1]*sizeof(float)); 
	free_cb_image( cbi );

		////// TIMING CODE //////////////////////
		QueryPerformanceCounter(&start_ticks_total);
		/////////////////////////////////////////

	printf ("Projecting Image:");
	// Project each image into the volume one at a time
	for(image_num = options->first_img;  image_num <= options->last_img;  image_num += options->skip_img)
	{
	printf (" %d\n", image_num);
	fflush(stdout);
			////// TIMING CODE //////////////////////
			#if defined (TIME_KERNEL)
				QueryPerformanceCounter(&start_ticks_io);
			#endif
			/////////////////////////////////////////

		// Load the current image
		cbi = get_image(options, image_num);
		if (cbi==NULL)
			continue;

		// Load dynamic kernel arguments
		kargs->img_dim.x = cbi->dim[0];
		kargs->img_dim.y = cbi->dim[1];
		kargs->ic.x = cbi->ic[0];
		kargs->ic.y = cbi->ic[1];
		kargs->nrm.x = cbi->nrm[0];
		kargs->nrm.y = cbi->nrm[1];
		kargs->nrm.z = cbi->nrm[2];
		kargs->sad = cbi->sad;
		kargs->sid = cbi->sid;
		for(i=0; i<12; i++)
			kargs->matrix[i] = (float)cbi->matrix[i];

		// Copy image pixel data & projection matrix to device Global Memory
		// and then bind them to the texture hardware.
		cudaMemcpy( dev_img, cbi->img, cbi->dim[0]*cbi->dim[1]*sizeof(float), cudaMemcpyHostToDevice );
		cudaBindTexture( 0, tex_img, dev_img, cbi->dim[0]*cbi->dim[1]*sizeof(float) );
		cudaMemcpy( dev_matrix, kargs->matrix, sizeof(kargs->matrix), cudaMemcpyHostToDevice );
		cudaBindTexture( 0, tex_matrix, dev_matrix, sizeof(kargs->matrix));

		// Free the current image 
		free_cb_image( cbi );

			////// TIMING CODE //////////////////////
			#if defined (TIME_KERNEL)
				QueryPerformanceCounter(&end_ticks_io);
				cputime.QuadPart = end_ticks_io.QuadPart- start_ticks_io.QuadPart;
				io_total += ((float)cputime.QuadPart/(float)ticksPerSecond.QuadPart);
				QueryPerformanceCounter(&start_ticks_kernel);
			#endif
			/////////////////////////////////////////

		// Invoke ze kernel  \(^_^)/
		// Note: cbi->img AND cbi->matrix are passed via texture memory
		//-------------------------------------
		kernel_fdk<<< dimGrid, dimBlock >>>(dev_vol,
						    kargs->img_dim,
						    kargs->ic,
						    kargs->nrm,
						    kargs->sad,
						    kargs->scale,
						    kargs->vol_offset,
						    kargs->vol_dim,
						    kargs->vol_pix_spacing,
						    blocksInY,
						    1.0f/(float)blocksInY);
		checkCUDAError("Kernel Panic!");

		#if defined (TIME_KERNEL)
			// CUDA kernel calls are asynchronous...
			// In order to accurately time the kernel
			// execution time we need to set a thread
			// barrier here after its execution.
			cudaThreadSynchronize();
		#endif

		// Unbind the image and projection matrix textures
		cudaUnbindTexture( tex_img );
		cudaUnbindTexture( tex_matrix );

			////// TIMING CODE //////////////////////
			#if defined (TIME_KERNEL)
				QueryPerformanceCounter(&end_ticks_kernel);
				cputime.QuadPart = end_ticks_kernel.QuadPart- start_ticks_kernel.QuadPart;
				kernel_total += ((float)cputime.QuadPart/(float)ticksPerSecond.QuadPart);
			#endif
			/////////////////////////////////////////
	}

	#if defined (VERBOSE)
		printf(" done.\n\n");
	#endif
	
	// Copy reconstructed volume from device to host
	cudaMemcpy( vol->img, dev_vol, vol->npix * vol->pix_size, cudaMemcpyDeviceToHost );
	checkCUDAError("Error: Unable to retrieve data volume.");

	
		////// TIMING CODE //////////////////////
		// Report Timing Data
		#if defined (TIME_KERNEL)
			QueryPerformanceCounter(&end_ticks_total);
			cputime.QuadPart = end_ticks_total.QuadPart- start_ticks_total.QuadPart;
			printf("========================================\n");
			printf ("[Total Execution Time: %.9fs ]\n", ((float)cputime.QuadPart/(float)ticksPerSecond.QuadPart));
			printf ("\tTotal Kernel  Time: %.9fs\n", kernel_total);
			printf ("\tTotal File IO Time: %.9fs\n\n", io_total);

			printf ("[Average Projection Time: %.9fs ]\n", ((float)cputime.QuadPart/(float)ticksPerSecond.QuadPart)/ (1+(options->last_img - options->first_img) / options->skip_img));
			printf ("\tAverage Kernel  Time: %.9fs\n", kernel_total/ (1+(options->last_img - options->first_img) / options->skip_img));
			printf ("\tAverage File IO Time: %.9fs\n\n", io_total/ (1+(options->last_img - options->first_img) / options->skip_img));
			printf("========================================\n");
		#else
			QueryPerformanceCounter(&end_ticks_total);
			cputime.QuadPart = end_ticks_total.QuadPart- start_ticks_total.QuadPart;
			printf("========================================\n");
			printf ("[Total Execution Time: %.9fs ]\n", ((float)cputime.QuadPart/(float)ticksPerSecond.QuadPart));
			printf("========================================\n");
		#endif
		/////////////////////////////////////////


	// Cleanup
	cudaFree( dev_img );
	cudaFree( dev_kargs );
	cudaFree( dev_matrix );
	cudaFree( dev_vol );	

	return 0;
}
//}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// FUNCTION: checkCUDAError() /////////////////////////////////////////////
void checkCUDAError(const char *msg)
{
    cudaError_t err = cudaGetLastError();
    if( cudaSuccess != err) 
    {
        fprintf(stderr, "CUDA ERROR: %s (%s).\n", msg, cudaGetErrorString( err) );
        exit(EXIT_FAILURE);
    }                         
}
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Vim Editor Settings ////////////////////////////////////////////////////
// vim:ts=8:sw=8:cindent:nowrap
///////////////////////////////////////////////////////////////////////////
