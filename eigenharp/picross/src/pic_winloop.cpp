
/*
 Copyright 2009 Eigenlabs Ltd.  http://www.eigenlabs.com

 This file is part of EigenD.

 EigenD is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 EigenD is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with EigenD.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <picross/pic_winloop.h>
#include <picross/pic_log.h>


#include <picross/pic_windows.h>

#define WM_MSG1 WM_USER+1
#define WM_MSG2 WM_USER+2

#define	WINDOW_TITLE	"TestWindow"
#define WINDOW_CLASS	"EigenlabsClass"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "GDI32.lib")

static class pic::winloop_t * winproc = NULL;

LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam );

void *  pic::winloop_t::CreateAWindow()
{
	HWND hwndConsole = GetConsoleWindow();
	
	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hwndConsole, GWL_HINSTANCE);

    WNDCLASS wc = {0};
    wc.hbrBackground =(HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hCursor = LoadCursor( hInstance, IDC_ARROW );
    wc.hIcon = LoadIcon( hInstance, IDI_APPLICATION );
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = WINDOW_CLASS;  
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (RegisterClass( &wc ) )
    {
		hwndWindow = CreateWindow(WINDOW_CLASS,WINDOW_TITLE,WS_OVERLAPPEDWINDOW,100,100, 300, 300,hwndConsole,NULL,hInstance, NULL );

		ShowWindow( (HWND)hwndWindow,SW_SHOWNORMAL); //SW_HIDE
		UpdateWindow( (HWND)hwndWindow );
	}

	winproc = this;

	return hwndConsole;
}

void pic::winloop_t::RunWindProc()
{
	MSG msg;
	while( GetMessage( &msg, (HWND)hwndWindow, 0, 0 ) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


#define IDT_TIMER1 666

LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
	static unsigned long a = 0;
	static HANDLE b = NULL;

    switch( message )
    {
        case WM_CREATE:
			
			pic::logmsg() << "Create Window";
			return 0;
            break;

		case WM_MSG1:
			//SetTimer(hwnd,IDT_TIMER1,5,(TIMERPROC) NULL);
			pic::logmsg() << "WM_MSG1";
			winproc->Open();
			return 0;

		case WM_MSG2:
			pic::logmsg() << "WM_MSG2";
			a = wparam;
			b = (HANDLE)lparam;
			winproc->audio_init();
			return 0;

		case WM_TIMER:
			if (wparam == IDT_TIMER1)
			{
				//winproc->audio_init();
				//pic::logmsg() << "Timer";
				return 0;
			}

        case WM_DESTROY:
			pic::logmsg() << "Destroy Window";
            PostQuitMessage(0);
			//KillTimer( hwnd, IDT_TIMER1 );    

			if (a && b)
			{
				// Close down notification thread
				PostThreadMessage( a, WM_QUIT, 0, 0 );
				WaitForSingleObject( b, INFINITE );
				CloseHandle( b );
			}
			else
			{
				pic::logmsg() << "Error notification thread unknown";
			}

            return 0;
            break;
    }

    return DefWindowProc( hwnd, message, wparam, lparam );
}


pic::winloop_t::winloop_t(unsigned realtime): thread_t(realtime)//, _loop(0)
{
    _gate.open();
}

pic::winloop_t::~winloop_t()
{
    stop();
}

void pic::winloop_t::stop()
{
    if(isrunning())
    {
		PostQuitMessage(0);
    }

    wait();
}

int pic::winloop_t::cmd(void *a1, void *a2)
{
	//PIC_ASSERT(_gate.shut());
	PostMessage((HWND)hwndWindow,WM_MSG1,(WPARAM)a1,(LPARAM)a2);
	//_gate.untimedpass();
	return 0; //_rv
}

void pic::winloop_t::cmd2(void *a1, void *a2)
{
	PostMessage((HWND)hwndWindow,WM_MSG2,(WPARAM)a1,(LPARAM)a2);
}

void pic::winloop_t::runloop_init()
{
}

void pic::winloop_t::runloop_start()
{
}

void pic::winloop_t::runloop_term()
{
}

int pic::winloop_t::runloop_cmd(void *, void *)
{
    return 0;
}

void pic::winloop_t::runloop_cmd2(void *, void *)
{
}

void pic::winloop_t::thread_init()
{
    init0();

    try
    {
        runloop_init();
    }
    catch(...)
    {
        term0();
        throw;
    }
}

void pic::winloop_t::thread_main()
{
    try
    {
        runloop_start();

        try
        {
			void * handle = CreateAWindow();
			RunWindProc();
        }
        catch(...)
        {
            try
            {
                runloop_term();
            }
            CATCHLOG()
            throw;
        }

        runloop_term();

    }
    CATCHLOG()
}


void pic::winloop_t::init0()
{
}

void pic::winloop_t::term0()
{
}

//void pic::winloop_t::_command2(WinRunLoopTimerRef, void *self)
//{
//    winloop_t *t = (winloop_t *)self;
//
//    try
//    {
//        //t->runloop_cmd2(t->_2arg1,t->_2arg2);
//    }
//    catch(...)
//    {
//    }
//}
//
//void pic::winloop_t::_command(WinRunLoopTimerRef, void *self)
//{
//    winloop_t *t = (winloop_t *)self;
//
//    try
//    {
//        //t->_rv=t->runloop_cmd(t->_arg1, t->_arg2);
//        t->_re=false;
//    }
//    catch(...)
//    {
//        t->_re=true;
//    }
//
//    t->_gate.open();
//}


