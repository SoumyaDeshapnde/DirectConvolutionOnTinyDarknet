__kernel void
conv(
__global uchar* A, 
__global uchar* B, 
__global uchar* C, 
__local uchar* Asub,
__local uchar* Bsub,
__local uchar* Csub,
const int outHSize,  const int outWSize,const int stride, const int filterSize,const int channels,const int kernels)
{
int tx = get_global_id(0); //Width
int ty = get_global_id(1);//Height
int product=0;

//__local uchar Bsub[filterSize*filterSize*channels];//filter*filter*channels
//__local uchar* Asub[outWSize*(filterSize)];//get windows of conv * rows needed
//__local uchar* Csub[outWSize*outHSize];

const int numWeights = kernels*filterSize*filterSize*channels;

for(int w = 0; w < numWeights; w++)
{
	Bsub[w] = B[w];
}

barrier(CLK_LOCAL_MEM_FENCE);
  //printf("** weights loaded**"); 
for(int k = 0; k < kernels; k++)//kernel
{	//producer
	for(int ch = 0; ch < channels; ch++)//channel
	{
		//__attribute__((xcl_dataflow))
		for (int i = 0; i <filterSize; i++)//width 
		{
			Asub[(i+tx)+ty*outWSize] = A[(i+tx)+ty*outWSize+ch*outWSize*outHSize];
			if(ty ==get_local_size(1)) 
			{Asub[(i+tx)+(ty+1)*outWSize] = A[(i+tx)+(ty+1)*outWSize+ch*outWSize*outHSize];}
			barrier(CLK_LOCAL_MEM_FENCE);
		//
		}//turn this into some sorta fifo
				product = 0; 
				//__attribute__((xcl_pipeline_loop))
				LOOP_1:for (int j = 0; j < filterSize; j++)//height
				{
					LOOP_2:for (int i = 0; i <filterSize; i++)//width 
					{
product = product + Asub[(i+tx)+(j+ty)*outWSize] * Bsub[i+j*filterSize+ch*filterSize*filterSize+k*filterSize*filterSize*channels];
					}
				}
			Csub[tx+ty*outWSize] = product+Csub[tx+ty*outWSize];
 //printf("\n** Printing  kernel id<%d \n", k); 
 //printf("\n** Printing  channel id<%d \n", ch);
	}

C[tx+ty*outWSize+k*outWSize*outHSize] =Csub[tx+ty*outWSize];//+k*outWSize*outHSize
barrier(CLK_LOCAL_MEM_FENCE);


}
}

__kernel void maxpool(__global unsigned char* output, __global unsigned char* image, int rows, int cols, int stride, int out_rows, int channel)
{	

	const int tx = get_global_id(0);
	const int ty = get_global_id(1);

	int m=0;
	int max_temp=image[(tx)+(ty)*rows];
	int ch = 0;

	for(ch=0;ch<channel;ch++)//channel
	{
		for (int j = 0; j < stride; j++)//height
		{
			for (int i = 0; i <stride; i++)//width 
			{	
				m=image[(i+tx)+(j+ty)*rows];
				if (m > max_temp)
        			{
					max_temp = m;
				}
			}
		}
	output[tx + out_rows * ty] = max_temp;
	}
}


