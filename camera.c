#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <sys/mman.h>

#define PATTEN_NUM 22
extern int errno;
struct buffer
{
    void* start;
    size_t length;
};

struct v4l2_standard standard;
struct v4l2_cropcap cropcap;
struct v4l2_crop crop;
struct v4l2_requestbuffers reqbuf;
struct buffer* buffers = NULL;
struct v4l2_format format;
enum v4l2_buf_type type;

int CameraSetup(int fd, int width, int height)
{
    int i;
    //step 2:get device capability
    struct v4l2_capability cap;
    memset(&cap,0,sizeof(cap));

    if(-1 == ioctl(fd,VIDIOC_QUERYCAP,&cap))
    {
        printf("Get cap error!\n");
        close(fd);
        return 0;
    }
    printf("driver:%s\ncard:%s\ninfo:%s\nversion:%d.%d.%d\n", 
        cap.driver,cap.card,cap.bus_info,(cap.version>>16)&0xff,(cap.version>>8)&0xff,(cap.version)&0xff);

    if(cap.capabilities&V4L2_CAP_VIDEO_CAPTURE != 0)
    {
        printf("Support capture!\n");
    }

    //step 3:choose video input
    int inputindex;

    if (-1 == ioctl(fd,VIDIOC_G_INPUT,&inputindex))
    {
        /* code */
        printf("VIDIOC_G_INPUT error!\n");
        close(fd);
        return 0;
    }
    struct v4l2_input input;
    memset(&input,0,sizeof(input));

    input.index = inputindex;

    if (-1 == ioctl(fd,VIDIOC_ENUMINPUT,&input))
    {
        /* code */
        printf("VIDIOC_ENUMINPUT failed!\n");
        close(fd);
        return 0;
    }
    printf("Current input:%s\n",input.name);

    //list video standards supported by the current input
    //for usb devices the standards make no sense.

    standard.index = 0;

    while(0 == ioctl(fd,VIDIOC_ENUMSTD,&standard))
    {
        if(standard.id&input.std)
        {
            printf("%s\n",standard.name );
        }
        standard.index++;
    }

    //step 4:set video system & frame format
    //cmd:V4L2_BUF_TYPE_VIDEO_CAPTURE

    //query info of video capture
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == ioctl(fd,VIDIOC_CROPCAP,&cropcap))
    {
        printf("Set crop cap error!\n");
        close(fd);
        return 0;
    }

    printf("default left:%d,top:%d,width:%d,height:%d,pixelaspect:%d/%d\n",
        cropcap.defrect.left,cropcap.defrect.top,cropcap.defrect.width,cropcap.defrect.height,cropcap.pixelaspect.numerator,cropcap.pixelaspect.denominator);

    printf("bounds left:%d,top:%d,width:%d,height:%d\n",
        cropcap.bounds.left,cropcap.bounds.top,cropcap.bounds.width,cropcap.bounds.height);

    //crop picture size

    // crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // crop.c.width = 480;
    // crop.c.height = 480;

    // if(-1 == ioctl(fd,VIDIOC_S_CROP,&crop))
    // {
    //     printf("VIDIOC_S_CROP error!ERRNO:%d\n",errno);
    //     close(fd);
    //     return 0;
    // }

    //set picture resolution

    
    memset(&format,0,sizeof(format));

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == ioctl(fd,VIDIOC_G_FMT,&format))
    {
        /* code */
        printf("VIDIOC_G_FMT error!\n");
        close(fd);
        return 0;
    }

    printf("Before set format:width:%d,height:%d,format:%d\n",format.fmt.pix.width,format.fmt.pix.height,format.fmt.pix.pixelformat);


    //traverse all format supported
    struct v4l2_fmtdesc formatsupport;
    memset(&formatsupport,0,sizeof(struct v4l2_fmtdesc));

    formatsupport.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int formatindex = 0;

    if (-1 == ioctl(fd,VIDIOC_ENUM_FMT,&formatsupport))
    {
        /* code */
        printf("VIDIOC_ENUM_FMT error!ERROR:%d\n",errno);
    }
    else
    {
        while(0 == ioctl(fd,VIDIOC_ENUM_FMT,&formatsupport))
        {
            /* code */
            printf("support format:%s,%d\n",formatsupport.description,formatsupport.pixelformat);
            formatsupport.index++;
        }
    } 

    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_NONE;//interlaced
    format.fmt.pix.colorspace = 8;

    if (-1 == ioctl(fd,VIDIOC_S_FMT,&format))
    {
        /* code */
        printf("VIDIOC_S_FMT error!\n");
        close(fd);
        return 0;
    }  

    printf("After set format:width:%d,height:%d,format:%d\n",format.fmt.pix.width,format.fmt.pix.height,format.fmt.pix.pixelformat);

    //exposure

    struct v4l2_control ctrl;
    memset(&ctrl,0,sizeof(ctrl));

    ctrl.id = V4L2_CID_BRIGHTNESS;
    if(-1 == ioctl(fd,VIDIOC_G_CTRL,&ctrl))
    {
        printf("error!errno:%d\n",errno);
    }
    else
    {
        printf("bright:%d\n",ctrl.value);
    }

    ctrl.id = V4L2_CID_CONTRAST;
    ctrl.value= 128;
    if(-1 == ioctl(fd,VIDIOC_S_CTRL,&ctrl))
    {
        printf("error!errno:%d\n",errno);
    }
    else
    {
        printf("contrast:%d\n",ctrl.value);
    }

    ctrl.id = V4L2_CID_SATURATION ;
    ctrl.value= 128;
    if(-1 == ioctl(fd,VIDIOC_S_CTRL,&ctrl))
    {
        printf("error!errno:%d\n",errno);
    }
    else
    {
        printf("saturation:%d\n",ctrl.value);
    }

    //step 5:apply for buffer from driver

    memset(&reqbuf,0,sizeof(reqbuf));

    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//frame format
    reqbuf.memory = V4L2_MEMORY_MMAP;//buffer is used for memory mapping I/O
    reqbuf.count = 5;//buffer number

    if(-1 == ioctl(fd,VIDIOC_REQBUFS,&reqbuf))
    {
        if(errno == EBUSY)
        {
            printf("The driver supports multiple opens and I/O is already in \
                progress,or reallocation of buffers was attempted although one \
                or more are still mapped.\n");
        }
        else if(errno == EINVAL)
        {
            printf("The buffer type(type field)or the requested I/O method(memory) \
                is not supported!\n");
        }
        else
        {
            printf("REQBUF ERROR!ERRNO:%d\n",errno);
        }
        close(fd);
        return 0;
    }

    //check get buffer number

    printf("Get buffer count:%d\n", reqbuf.count);
    if(reqbuf.count<4)
    {
        printf("Insufficient buffer memory!\n");
        close(fd);
        return 0;
    }

    
    buffers = (struct buffer*)calloc(reqbuf.count,sizeof(struct buffer));//allocate req.count pointers in user space
    if (buffers == NULL)
    {
        /* code */
        printf("allocate error!\n");
        close(fd);
        return 0;
    }

    //step 6:map frame buffer to user space
    for (i = 0; i < reqbuf.count; ++i)
    {
        /* code */
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == ioctl(fd,VIDIOC_QUERYBUF,&buf))
        {
            /* code */
            free(buffers);
            close(fd);
            return 0;

        }

        buffers[i].length = buf.length;//remember for munmap()
        buffers[i].start = (char*)mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buf.m.offset);

        if (MAP_FAILED == buffers[i].start)
        {
            /* code */
            printf("mmap error!\n");
            free(buffers);
            close(fd);
            return 0;
        }
        
    }

    //step 7:before start data flow,put buffer frame into buffer queue

    for (i = 0; i < reqbuf.count; ++i)
    {
        /* code */
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        //put buffer frame into buffer queue
        if (-1 == ioctl(fd,VIDIOC_QBUF,&buf))
        {
            /* code */
            printf("VIDIOC_QBUF error!\n");
            free(buffers);
            close(fd);
            return 0;
        }       
    }

    //step 8:start data flow

    

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == ioctl(fd,VIDIOC_STREAMON,&type))
    {
        printf("VIDIOC_STREAMON error!\n");
        free(buffers);
        close(fd);
        return 0;
    }
    return 1;
}

int CameraGet(int fd, struct v4l2_buffer *imagebuf)
{
	memset(imagebuf,0,sizeof(struct v4l2_buffer));

    imagebuf -> type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imagebuf -> memory = V4L2_MEMORY_MMAP;
    if(-1 == ioctl(fd,VIDIOC_DQBUF,imagebuf)){
        printf("VIDIOC_DQBUF error!\n");
        free(buffers);
        close(fd);
        return 0;
    }
    return 1;
}

void CameraPut(int fd, struct v4l2_buffer *imagebuf)
{
    ioctl(fd,VIDIOC_QBUF,imagebuf);
}

int CameraStop(int fd)
{

	int i;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == ioctl(fd,VIDIOC_STREAMOFF,&type))
    {
        printf("VIDIOC_STREAMOFF error!\n");
        free(buffers);
        close(fd);
        return 0;
    }

    for (i = 0; i < reqbuf.count; ++i)
    {
        /* code */
        {
            munmap(buffers[i].start,buffers[i].length);           
        }
    }

    return 1;
}

int main()
{
    int i;
    int fd;
    int fdSerial;
    
    

    FILE *fpre;
    FILE *fout[PATTEN_NUM];


    char binaryFlag[1] = {0xff};
	char grayFlag[1] = {0xfe};  
  	char pattern[22] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x61, 
                        0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c};
    char buff[10];
    memset(buff,0,10);
    
    //step 1:open usb camera device
    fd = open("/dev/video2",O_RDWR);

    if(fd <0)
    {
        printf("Cannot open camera!\n");
        return 0;
    }

    if((fdSerial=open_port(fdSerial,1))<0){
	    perror("open_port error");
	    return;
	}
	if((i=set_opt(fdSerial,115200,8,'N',1))<0){
		perror("set_opt error");
		return;
	}

    CameraSetup(fd, 1280, 720);

    //step 9:image process

    struct v4l2_buffer imagebuf;
    // memset(&imagebuf,0,sizeof(imagebuf));

    // imagebuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // imagebuf.memory = V4L2_MEMORY_MMAP;
    // imagebuf.index = 0;

    int j=0;
    char nameBuf[30];
    memset(nameBuf, 0, 30);

    for(i = 0; i < PATTEN_NUM; i++){
        sprintf(nameBuf, "image/dataout%d.yuv", i);
        // printf("%s",nameBuf);
        if((fout[i] = fopen(nameBuf,"w")) == NULL)
        {
            printf("Cannot open dataout[%d].yuv!\n", i);
            free(buffers);
            close(fd);
            return 0;
        } 
    }   
    
    
    CameraGet(fd, &imagebuf);
    CameraGet(fd, &imagebuf);
    CameraGet(fd, &imagebuf);
    CameraGet(fd, &imagebuf);
    CameraGet(fd, &imagebuf);

    if((fpre = fopen("image/pre.yuv","w")) == NULL){
            printf("Cannot open pre.yuv!\n", i);
            free(buffers);
            close(fd);
            return 0;
    }

    for (j = 0; j < 10; ++j){
        CameraPut(fd, &imagebuf);
        CameraGet(fd, &imagebuf);
        fwrite(buffers[imagebuf.index].start,1,format.fmt.pix.width*format.fmt.pix.height*2,fpre);
    }
	while(1){
	    printf("please input '1' for binary, '2' for gray, '3' for get image\n");
	  	printf("<< ");
	  	scanf("%s",buff);


	    if(strcmp(buff, "1") == 0){
	    	write(fdSerial, binaryFlag, 1);
	      	}

	    else if(strcmp(buff, "2") == 0){
	    	write(fdSerial, grayFlag, 1); 	
	    }

	    else if(strcmp(buff, "3") == 0){
	    	for(i = 0; i < PATTEN_NUM; i++){
		        write(fdSerial, pattern + i, 1);
		        //usleep(10000);
		        CameraPut(fd, &imagebuf);
        		CameraGet(fd, &imagebuf);
        		fwrite(buffers[imagebuf.index].start,1,format.fmt.pix.width*format.fmt.pix.height*2,fout[i]);
		        //usleep(100000);
	      	}
	    }

	  	memset(buff,0,10);
	 }
    

    CameraStop(fd);

    for(j = 0; j < PATTEN_NUM; j++){
        fclose(fout[j]);
    }
    
    free(buffers);
    close(fd);
    return 0;
}
