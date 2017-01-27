#include <libusb.h>
#include <unistd.h>

#define HDR_SZ  0x10
#define DATA_SZ (20 * 0x4000)

#define WIDTH   960
#define HEIGHT  160
#define LINEBUF (DATA_SZ / HEIGHT)

#define RGB_H(r,g,b) ( ((g & 0x07) << 5) | (r & 0x1F) ) 
#define RGB_L(r,g,b) ( ((b & 0x1F) << 3) | (g & 0xE0) )

#define RB 5

int main(int argc, char** argv) {
    libusb_device_handle *handle;
    libusb_init(NULL);
    handle = libusb_open_device_with_vid_pid(NULL, 0x2982,0x1967);

    static unsigned char headerPkt[HDR_SZ] = { 0xef, 0xcd, 0xab, 0x89, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static unsigned char dataPkt[DATA_SZ];

    int off_l=0,off_h=0;
    int on_l = RGB_L(0x00,0x00,0xf);
    int on_h = RGB_H(0x00,0x00,0xf);
    for(int x=0;x<sizeof(dataPkt);x+=2) {
        int col = (x % LINEBUF) / 2;
        int row = (x / LINEBUF);
        dataPkt[x]= (col <10 || col >= (WIDTH - 10)) ? on_h : ( row < 10 || row >= (HEIGHT - 10) ? on_h : off_h);
        dataPkt[x+1]= (col <10 || col >= (WIDTH - 10)) ? on_l : ( row < 10 || row >= (HEIGHT - 10) ? on_l : off_l);
    }
    
    libusb_claim_interface(handle, 0x00);

    int tfrsize = 0;

    for(int i=0;i<60;i++) {
        libusb_bulk_transfer(handle, 0x01, headerPkt, sizeof(headerPkt), &tfrsize, 1000);
        libusb_bulk_transfer(handle, 0x01, dataPkt, sizeof(dataPkt), &tfrsize, 1000);
        usleep(1000000);
    }
    
    libusb_release_interface(handle, 0x00);
    libusb_close(handle);
    libusb_exit(NULL);

    return 0;
}


