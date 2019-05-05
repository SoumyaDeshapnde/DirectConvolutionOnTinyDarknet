#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <CL/cl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h> 
#include <stdint.h>
#include <time.h>




long LoadOpenCLKernel(char const* path, char **buf)
{
	FILE  *fp;
	size_t fsz;
	long   off_end;
	int    rc;

	/* Open the file */
	fp = fopen(path, "r");
	if( NULL == fp ) 
	{
		return -1L;
	}

	/* Seek to the end of the file */
	rc = fseek(fp, 0L, SEEK_END);
	if( 0 != rc ) 
	{
		return -1L;
	}

	/* Byte offset to the end of the file (size) */
	if( 0 > (off_end = ftell(fp)) ) 
	{
		return -1L;
	}
	fsz = (size_t)off_end;

	/* Allocate a buffer to hold the whole file */
	*buf = (char *) malloc( fsz+1);
	if( NULL == *buf ) 
	{
		return -1L;
	}

	/* Rewind file pointer to start of file */
	rewind(fp);

	/* Slurp file into buffer */
	if( fsz != fread(*buf, 1, fsz, fp) ) 
	{
		free(*buf);
		return -1L;
	}

    /* Close the file */
	if( EOF == fclose(fp) ) 
	{
	        free(*buf);
		return -1L;
	}


	/* Make sure the buffer is NUL-terminated, just in case */
	(*buf)[fsz] = '\0';

	/* Return the file size */
	return (long)fsz;
}


int main(int argc, char** argv)
{
	printf("\nRunning!\n");

	int err;                            	// error code returned from api calls
	int W=56;
	int H=56;
	int C=16;
	int N=3;
	int files=1;
	int kernels=1;
	char buff[200];
	int sizeFrame = W*H*C*sizeof(char);
	uint8_t *frame;
	size_t sizeFrameOUT = H*W*C*sizeof(uint8_t)*files;//if int this would brake
	uint8_t *frameOUT;
	frameOUT = (uint8_t*)malloc(sizeFrameOUT);
	int i=0;
	int f=0;
	int x,y,j,k;
	int BATCH=0;
	//FM dimensions*
	//weight dimensions
	int WB =C;
	int HB =kernels*N*N;

	for(k=0;k<(H);k++)//height
	{
		for (j=0;j<(W);j++)//width
		{
			for(i=0;i<(C);i++)//channel
			{
				//this is altered so that the entire image is just store naively in an array
				frameOUT[j+k*W+i*W*H+BATCH*C*W*H] =j+k*W+i*W*H ;
			}
		}
	}


	uint8_t *Weights;
	uint8_t n;
	uint8_t *weights;
	size_t sizeWeights = N*N*C*kernels*sizeof(uint8_t);
	weights = (uint8_t *)malloc(sizeWeights); 
	int wK;
	int wCH;
	int wH;
	int wW;
	for(wK=0;wK<(kernels);wK++)//height
	{
		for(wCH=0;wCH<(C);wCH++)//height
		{
			for (wH=0;wH<(N);wH++)//width
			{
				for(wW=0;wW<(N);wW++)//channel
				{	
					//fscanf(wp, "%s", Wbuff);
					//n = (uint8_t)atoi(Wbuff);
					weights[wW+wH*N+wCH*N*N+wK*N*N*C]=wW+wH*N;
				}
			}
		}
	}	




	cl_device_id device_id;             	// compute device id 
	cl_context context;                 	// compute context
	cl_command_queue commands;          	// compute command queue
	cl_program program;                 	// compute program
	cl_kernel kernel;                   	// compute kernel
	//cl_program program2;                 // compute program
        cl_kernel kernel2;                   // compute kernel
 
	printf("Initializing OpenCL device...\n"); 

	cl_uint dev_cnt = 0;
	clGetPlatformIDs(0, 0, &dev_cnt);
	
	cl_platform_id platform_ids[100];
	clGetPlatformIDs(dev_cnt, platform_ids, NULL);

	err = clGetDeviceIDs(platform_ids[0],CL_DEVICE_TYPE_ALL, 1, &device_id, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to create a device group!\n");
		return EXIT_FAILURE;
	}
  
	// Create a compute context 
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (!context)
	{
		printf("Error: Failed to create a compute context!\n");
		return EXIT_FAILURE;
	}

	// Create a command commands
	commands = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
	if (!commands)
	{
		printf("Error: Failed to create a command commands!\n");
		return EXIT_FAILURE;
	}
//////////////////////////////////////////////////////////////////////////
//kernel stuff [including]: create program from source,build exe,create comput kernel
/////////////////////////////////////////////////////////////////////////


printf("\nOKAY... done with that\n"); 


//CONV:

// Create the compute program2 from the source file
   char *KernelSource2;
   size_t lFileSize2;
   lFileSize2 = LoadOpenCLKernel("./xclbin/conv.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.xclbin", &KernelSource2);
   if( lFileSize2 < 0L ) 
   {
	perror("File read failed");
	return 1;
   }
printf("\nconv kernel source gotten\n"); 
   program = clCreateProgramWithBinary(context, 1, &device_id,&lFileSize2,(const unsigned char **) &KernelSource2, NULL, &err);
   if (!program)
   {
	printf("Error: Failed to create compute program2! error: %d\n",err);
	return EXIT_FAILURE;
   }
printf("\nprogram for conv created\n"); 
   // Build the program executable
   err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
   if (err != CL_SUCCESS)
   {
       size_t len;
       char buffer[2048];
       printf("Error: Failed to build program2 executable!\n");
       clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
       printf("%s\n", buffer);
       exit(1);
   }
printf("\nbuild done\n"); 
   kernel2 = clCreateKernel(program, "conv", &err);
   if (!kernel || err != CL_SUCCESS)
   {
	printf("Error: Failed to create compute kernel!\n");
	exit(1);
   }

printf("\nconv kernel loaded\n");

	// Create the compute kernel in the program we wish to run
	//kernel name
	kernel = clCreateKernel(program, "maxpool", &err);
	if (!kernel || err != CL_SUCCESS)
	{
		printf("Error: Failed to create compute kernel!\n");
		exit(1);
	}
printf("\npool kernel loaded\n");



 
//CONV_DONE
/////////////////////////////////////////////////

//POOL:
/*
printf("\nnow to try pooling...\n"); 
	// Create the compute program from the source file
	char *KernelSource;
	size_t lFileSize;
	lFileSize = LoadOpenCLKernel("./xclbin/maxpool.sw_emu.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.xclbin", &KernelSource);
printf("\nconv kernel source gotten\n"); 

	if( lFileSize < 0L ) {perror("File read failed");return 1;}

	program = clCreateProgramWithBinary(context, 1, &device_id,&lFileSize,(const unsigned char **) &KernelSource, NULL, &err);

printf("\npool program made\n"); 

	if (!program)
	{
		printf("Error: Failed to create compute program! error: %d\n",err);
		return EXIT_FAILURE;
	}

	// Build the program executable
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];
		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		exit(1);
	}
printf("\nbuild exe done\n"); 

	// Create the compute kernel in the program we wish to run
	//kernel name
	kernel = clCreateKernel(program, "maxpool", &err);
	if (!kernel || err != CL_SUCCESS)
	{
		printf("Error: Failed to create compute kernel!\n");
		exit(1);
	}
printf("\npool kernel loaded\n");
*/
//POOL_DONE
//memory stuff
//TODO:check what is used
	//Device image input

	//pool1


	//Host image input

	//uint8_t* h_A = frameOUT;
	//h_B= weights;
	
	//Allocate host memory for the result C

	


	//unsigned char* h_input0 = (unsigned char*) malloc(HEIGHT * WIDTH * Channel * sizeof(unsigned char));
	
	//Host image output














//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//kernel stuff done, no into usage:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//0th CONV KERNEL
//set up the memory trasfer


	cl_mem d_input0FM;
	size_t mem_size_A = sizeFrameOUT;
	cl_mem d_input0W;
	size_t mem_size_B =sizeWeights;
	cl_mem d_output0FM;
	size_t size_C = ((W-N)+1)*((H-N)+1)*files*kernels;
	size_t mem_size_C = sizeof(uint8_t) * size_C;
	uint8_t* h_output1 = (uint8_t*) malloc(mem_size_C); 	  
   d_input0FM = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeFrameOUT, frameOUT, &err);
   d_input0W = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeWeights, weights, &err);
   d_output0FM = clCreateBuffer(context, CL_MEM_READ_WRITE, mem_size_C, NULL, &err);

printf("\nconv mem loaded\n");

   if (!d_input0FM || !d_input0W || !d_output0FM)
   {
       printf("Error: Failed to allocate device memory!\n");
       exit(1);
   }    
//set kernel parameters
	size_t localWorkSize[2], globalWorkSize[2]; 
 	int outHSize=((H-N)+1);
 	int outWSize=((W-N)+1);
	int stride=1;

   err |= clSetKernelArg(kernel2, 0, sizeof(cl_mem), (void *)&d_input0FM);
   err |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), (void *)&d_input0W);
   err |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), (void *)&d_output0FM);
   err |= clSetKernelArg(kernel2, 3,  N*N*C*sizeof(uint8_t), NULL);
   err |= clSetKernelArg(kernel2, 4,  outWSize*N*sizeof(uint8_t), NULL);
   err |= clSetKernelArg(kernel2, 5,  outWSize*2*sizeof(uint8_t), NULL);
   err |= clSetKernelArg(kernel2, 6, sizeof(int), (void *)&outHSize);
   err |= clSetKernelArg(kernel2, 7, sizeof(int), (void *)&outWSize);
   err |= clSetKernelArg(kernel2, 8, sizeof(int), (void *)&stride);
   err |= clSetKernelArg(kernel2, 9, sizeof(int), (void *)&N);
   err |= clSetKernelArg(kernel2, 10, sizeof(int), (void *)&C);
  err |= clSetKernelArg(kernel2, 11, sizeof(int), (void *)&kernels);
   if (err != CL_SUCCESS)
   {
       printf("Error: Failed to set kernel arguments! %d\n", err);
       exit(1);
   }
 
   localWorkSize[0] = 54;
   localWorkSize[1] = 2;// cant go bigger, should be (F-1)?, parallel weight loading?, thread assignment
   globalWorkSize[0] = outWSize;
   globalWorkSize[1] = outHSize;
printf("\nconv args set\n");
   //call kernel
   err = clEnqueueNDRangeKernel(commands, kernel2, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
   if (err != CL_SUCCESS)
   {	
	printf("Error: Failed to execute kernel! %d\n", err);
	exit(1);
   }
printf("\nconv called\n");

 
   //d_out->host_out
   err = clEnqueueReadBuffer(commands, d_output0FM, CL_TRUE, 0, mem_size_C, h_output1, 0, NULL, NULL);
	 if (err != CL_SUCCESS)
	{
		printf("Error: Failed to read output array! %d\n", err);
		exit(1);
	}

  printf("\nconv done out put being read\n");
	clReleaseMemObject(d_input0FM);
	clReleaseMemObject(d_input0W);
	clReleaseMemObject(d_output0FM);
	free(frameOUT);
	free(weights);

//0th CONV KERNEL DONE
//////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//1st max_pool

int Stride =2;
int HEIGHT_FM= outHSize;
int WIDTH_FM= outWSize;
int HEIGHT_ACT= HEIGHT_FM/Stride;
int WIDTH_ACT= WIDTH_FM/Stride;
int ChannelFM = 1;
int ChannelACT = ChannelFM;

	cl_mem d_input1FM;
	cl_mem d_output1FM;
	unsigned char* h_output2;
	int rows = WIDTH_FM;
	int cols = HEIGHT_FM;
	int outputrow = rows/stride;

	h_output2 = (unsigned char*) malloc((outputrow) * (outputrow) * ChannelACT * sizeof(unsigned char));
	printf("\n1st maxpooling: 112x112x32 to 56x56x32\n");
	//For 1st maxpooling: 112x112x32 to 56x56x32
	// Create the input and output arrays in device memory for our calculation

	size_t mem_size_IN = sizeof(unsigned char) *HEIGHT_FM * WIDTH_FM * ChannelFM;
	size_t mem_size_Out = sizeof(unsigned char) * HEIGHT_ACT * WIDTH_ACT * ChannelACT;
	d_input1FM = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,mem_size_IN, h_output1 ,&err);
	d_output1FM = clCreateBuffer(context, CL_MEM_WRITE_ONLY,mem_size_Out, NULL,&err);
	  printf("\npooling memory initialized\n");
 
	if (!d_output1FM|| !d_input1FM)
	{
		printf("Error: Failed to allocate device memory!\n");
		exit(1);
	}    

	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&d_output1FM);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&d_input1FM);
	err |= clSetKernelArg(kernel, 2, sizeof(int), (void *)&rows);
	err |= clSetKernelArg(kernel, 3, sizeof(int), (void *)&cols);
	err |= clSetKernelArg(kernel, 4, sizeof(int), (void *)&stride);
	err |= clSetKernelArg(kernel, 5, sizeof(int), (void *)&outputrow);
	err |= clSetKernelArg(kernel, 6, sizeof(int), (void *)&ChannelACT);
 
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to set kernel arguments! %d\n", err);
		exit(1);
	}

	//Set local and global workgroup sizes
	localWorkSize[0] = 2;
	localWorkSize[1] = 2;
	globalWorkSize[0] = 28;
	globalWorkSize[1] = 28;
	 printf("\npooling args set\n");
	err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to execute kernel! %d\n", err);
		exit(1);
	}

	err = clEnqueueReadBuffer(commands, d_output1FM, CL_TRUE, 0, mem_size_Out, h_output2, 0, NULL, NULL);
	 printf("\npooling compute and read done\n");
	if (err != CL_SUCCESS)
	{printf("Error: Failed to read output array! %d\n", err);exit(1);}	

	clReleaseMemObject(d_input1FM);
	clReleaseMemObject(d_output1FM);
	free(h_output1);
 printf("\npooling data cleared\n");
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	//clReleaseProgram(program2);
	clReleaseKernel(kernel2);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);	
   	printf("direct-conv completed...\n"); 

	free(h_output2);
	if (err != CL_SUCCESS)
	{printf("Error: Failed to execute kernel! %d\n", err);exit(1);}

return 0;
}
