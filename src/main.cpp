#include "drawing/sextantBlocks.hpp"
#include <clocale>
#include <csignal>
#include <exception>
#include <notcurses/notcurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <EGL/egl.h>
#include <GL/gl.h>
#include <glm/ext/vector_uint2.hpp>
#include "raytracer/raytracer.hpp"
#include "raytracer/eglconf.hpp"
#include "rasterizer/rasterizer.hpp"

static_assert(CHAR_BIT == 8, "WTF are you compiling this on?!");

void termHandler() {
	auto error = std::current_exception();
	if (error != NULL) {
		try {
			std::rethrow_exception(error);
		} catch (std::exception& e) {
			std::cerr << "Exception encountered: " << e.what() << '.';
		}
	} else {
		std::cerr << "Terminate called.";
	}
 	std::cerr << " Stacktrace follows.\n";

	void* trace_elems[25];
	int trace_elem_count = backtrace(trace_elems, 25);
	char** as_strings = backtrace_symbols(trace_elems, trace_elem_count);

	for (int i = 0; i < trace_elem_count; i++) {
		std::cerr << as_strings[i] << '\n';
	}
	std::cerr << std::flush;

	free(as_strings);
	abort();
}

bool EXIT_REQUESTED = false;

void sigHandle([[maybe_unused]] int sig) {
	// try exiting nicely once, then hard abort
	if (not EXIT_REQUESTED) {
		EXIT_REQUESTED = true;
	} else {
		abort();
	}
}

int main() {
	std::set_terminate(termHandler);

	// make interrups exit nicely
	signal(SIGINT, sigHandle);
	signal(SIGTERM, sigHandle);

	setlocale(LC_ALL, "");
	// notcurses* nc = notcurses_core_init(NULL, stdout);
	// ncplane* stdplane = notcurses_stdplane(nc);

	// WindowedDrawing finalDrawing(stdplane);
	// auto render = std::bind(notcurses_render, nc);
	// rayRenderLoop(finalDrawing, EXIT_REQUESTED, render);

	EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint major, minor;
	eglInitialize(eglDisplay, &major, &minor);
	EGLint numConfigs;
	EGLConfig eglConfig;
	eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &numConfigs);
	EGLSurface eglSurface = eglCreatePbufferSurface(eglDisplay, eglConfig, pbattr);
	eglBindAPI(EGL_OPENGL_API);
	EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, NULL);
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

	glClearColor(1.0, 0.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glm::uvec2 readSize{4, 4};
	uchar* pixels = (uchar*) calloc(readSize.x * readSize.y, sizeof(uchar) * 4);
	glReadPixels(0, 0, readSize.x, readSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	for (size_t x = 0; x < readSize.x; x++) {
		for (size_t y = 0; y < readSize.y; y++) {
			std::print(std::cout, "{} {} {} \t", pixels[(x*y + y) * 4],
				pixels[(x*y + y) * 4 + 1], pixels[(x*y + y) * 4 + 2], pixels[(x*y + y) * 4 + 3]);
		}
		std::println(std::cout, "");
	}
	free(pixels);

	eglTerminate(eglDisplay);
	// notcurses_stop(nc);
	return 0;
}

