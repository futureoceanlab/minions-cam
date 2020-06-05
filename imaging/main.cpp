#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include "tiffio.h"
#include "tcamcamera.h"

//#include <fstream>
//#include <omp.h>


#define O_BINARY 0

// Create a custom data structure to be passed to the callback function. 
typedef struct
{
    int ID;
    int ImageCounter;
    bool SaveNextImage;
    bool busy;
} CUSTOMDATA;


using namespace gsttcam;
using namespace std;


const string SN[2] = {"15410110", "41810422"}; //"43810451"}; //

int k = 0;

int writeTiff(unsigned char *buf, char* outFileName);
struct timespec now;


////////////////////////////////////////////////////////////////////
// List available properties helper function.
void ListProperties(TcamCamera &cam)
{
    // Get a list of all supported properties and print it out
    auto properties = cam.get_camera_property_list();
    std::cout << "Properties:" << std::endl;
    for(auto &prop : properties)
    {
        std::cout << prop->to_string() << std::endl;
    }
}

void setCameraProperty(TcamCamera &cam, string property, int value)
{
    shared_ptr<Property> Property = NULL;
	try
	{
		Property = cam.get_property(property);
	}
	catch(std::exception &ex)    
	{
		printf("Error %s : %s\n",ex.what(), property.c_str());
	}

    if( Property != NULL){
        Property->set(cam,value);
        cout << property << " set to: " << value << endl;
    } 
	else 
	{
		cout << property << " setting failed!" << endl;
	}
}
////////////////////////////////////////////////////////////////////
// Callback called for new images by the internal appsink
GstFlowReturn new_frame_cb(GstAppSink *appsink, gpointer data)
{
    int width, height ;
    const GstStructure *str;

    // Cast gpointer to CUSTOMDATA*
    CUSTOMDATA *pCustomData = (CUSTOMDATA*)data;
    // if( !pCustomData->SaveNextImage)
    //     return GST_FLOW_OK;
    // pCustomData->SaveNextImage = false;

    pCustomData->ImageCounter++;
    //printf("img%05d_%d\n", k, pCustomData->ID);
    //k++;
    // The following lines demonstrate, how to acces the image
    // data in the GstSample.
    GstSample *sample = gst_app_sink_pull_sample(appsink);

    GstBuffer *buffer = gst_sample_get_buffer(sample);

    GstMapInfo info;

    gst_buffer_map(buffer, &info, GST_MAP_READ);
    
    if (info.data != NULL) 
    {
        // info.data contains the image data as blob of unsigned char 
		clock_gettime(CLOCK_MONOTONIC, &now);
        GstCaps *caps = gst_sample_get_caps(sample);
        // Get a string containg the pixel format, width and height of the image        
        str = gst_caps_get_structure (caps, 0);    

        if( strcmp( gst_structure_get_string (str, "format"),"GRAY8") == 0)  
        {
            // Now query the width and height of the image
            //gst_structure_get_int (str, "width", &width);
            //gst_structure_get_int (str, "height", &height);

            // Create a cv::Mat, copy image data into that and save the image.
            //pCustomData->frame.create(height,width,CV_8U);

            // memcpy( pCustomData->frame.data, info.data, width*height);
            // memcpy( pCustomData->frame.data, img.data, width*height);
            //printf("img%05d_%d w: %d h: %d\n", k, pCustomData->ID, width, height);
            // // cout <<"HE" <<endl;
            char ImageFileName[256];
			//cout << info.size << endl;
	     
            sprintf(ImageFileName,"/home/pi/data/image%05d_%d_%d_%d.tif", k, pCustomData->ID, now.tv_sec, now.tv_nsec);
            //cout << ImageFileName << endl;
            writeTiff(info.data, ImageFileName);
            /*fstream myFile;
            myFile.open(ImageFileName, fstream::out);
            myFile << info.data;
            myFile.close();*/
            k++;
        }
    }
    
    // Calling Unref is important!
    gst_buffer_unmap (buffer, &info);
    gst_sample_unref(sample);

    // Set our flag of new image to true, so our main thread knows about a new image.
    return GST_FLOW_OK;
}

int run_camera(string sn, int id)
{
    // Declare custom data structure for the callback
    CUSTOMDATA CustomData;

    CustomData.ImageCounter = 0;
    CustomData.SaveNextImage = false;
    CustomData.ID = id;
    printf("Tcam OpenCV Image Sample\n");

    // Open camera by serial number
    // TcamCamera cam("43810451");
    TcamCamera cam(sn); 

    // Set video format, resolution and frame rate
    // cam.set_capture_format("GRAY8", FrameSize{2592,1944}, FrameRate{15,2});
    cam.set_capture_format("GRAY8", FrameSize{2592,1944}, FrameRate{15, 2});
    // Register a callback to be called for each new frame
    cam.set_new_frame_callback(new_frame_cb, &CustomData);
    // Start the camera
	setCameraProperty(cam, "Exposure Auto", 0);
	setCameraProperty(cam, "Gain Auto", 0);
	setCameraProperty(cam, "Exposure", 1500); //us
	setCameraProperty(cam, "Gain", 16);
	setCameraProperty(cam, "Trigger Global Reset Shutter", 1);
	setCameraProperty(cam, "Trigger Mode", 1);
    
    //ListProperties(cam);

    cam.start();
    sleep(100000);
    cam.stop();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("Need Serial number");
        return 0;
    }
    int sn_i = atoi(argv[1]);
    int id = atoi(argv[2]);
    run_camera(SN[sn_i], id);
    return 0;
}



int writeTiff(unsigned char *buf, char* outFileName)
{
	int	fd, c;
	uint32_t row, col, band;
    uint32_t linebytes, bufsize;
	TIFF	*out;

	uint32_t width = 2592, length = 1944;
    uint32_t nbands = 1, rowsperstrip=3; /* number of bands in input image*/
	off_t	hdr_size = 0;		    /* size of the header to skip */
	unsigned char *buf1 = NULL;


	uint16_t	photometric = PHOTOMETRIC_MINISBLACK;
	uint16_t	config = PLANARCONFIG_CONTIG;
	uint16_t	fillorder = FILLORDER_LSB2MSB;
	TIFFDataType dtype = TIFF_BYTE;
    uint16_t compression = COMPRESSION_PACKBITS;
    int16_t depth = TIFFDataWidth(dtype); /* bytes per pixel in input image */


	out = TIFFOpen(outFileName, "w");
	if (out == NULL) {
		fprintf(stderr, "%s: Cannot open file for output.\n", outFileName);
		return (-1);
	}

	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, length);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, nbands);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, depth * 8);
	TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
    TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

    linebytes = width * nbands * depth;
	bufsize = width * nbands * depth;
	//buf1 = (unsigned char *)_TIFFmalloc(bufsize);

	/*rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip);
	if (rowsperstrip > length) {
		rowsperstrip = length;
	}*/
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip );

	lseek(fd, hdr_size, SEEK_SET);		/* Skip the file header */
	for (row = 0; row < length; row++) {
		buf1 = buf + row * bufsize;	
		if (TIFFWriteScanline(out, buf1, row, 0) < 0) {
			fprintf(stderr,	"%s: scanline %lu: Write error.\n",
                    outFileName, (unsigned long) row);
			break;
		}
	}
	TIFFClose(out);
	return (0);
}

