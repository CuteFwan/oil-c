#include <Python.h>
#include <Magick++.h> 
#include <iostream> 
#include <map>
#include <math.h>
#include <vector>

using namespace std; 
using namespace Magick; 

class coord {
    public:
    int x;
    int y;
    coord(int a, int b) {
        x = a;
        y = b;
    }
    coord offset(int a, int b) {
        return coord(x + a, y + b);
    }
    bool inbound() {
        return x >= 0 && y >= 0;
    }
};


bool is_square(int n) {
    if (n < 0) {
        return false;
    }
    int root = round(sqrt(n));
    return n == root * root;
}


static PyObject* oil(PyObject *self, PyObject *args) {
    Py_buffer bytes;
    PyObject *k_buffer;
    int k_length;
    int k_sq;
    vector<coord> kernel;
    int levels = 20;
    
    if (!PyArg_ParseTuple(args, "y*O|i", &bytes, &k_buffer, &levels)) {
        return NULL;
    }
    

    
    
    InitializeMagick("h");
    Blob blob(bytes.buf, bytes.len);
    Image image;
    image.read(blob);
    Image out;
    out.read(blob);
    
    Py_BEGIN_ALLOW_THREADS
    
    k_length = PyObject_Length(k_buffer);
    k_sq = sqrt(k_length);
    if (!is_square(k_length)) {
        return NULL;
    }
    
    for (int i = 0; i < k_sq; i++) {
        for (int j = 0; j < k_sq; j++) {
            PyObject *item;
            item = PyList_GetItem(k_buffer, i*k_sq+j);
            if (!PyLong_Check(item)) {
                cout << "something went wrong" << endl;
                continue;
            }
            if (PyLong_AsLong(item) == 1) {
                coord c(j - k_sq/2, i - k_sq/2);
                kernel.push_back(c);
            }
        }
    }
    
    int w = (int)image.baseColumns();
    int h = (int)image.baseRows();
    
    image.modifyImage();
    out.modifyImage();
    
    Quantum *pixels = image.getPixels(0,0,w,h);
    Quantum *opixels = out.getPixels(0,0,w,h);
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            map<int, int> avgR;
            map<int, int> avgG;
            map<int, int> avgB;
            map<int, int> avgA;
            map<int, int> count;
            
            for (int k = 0; k < kernel.size(); k++) {
                int x_sub = kernel[k].x + x;
                int y_sub = kernel[k].y + y;
                if (x_sub >= w || x_sub < 0 || y_sub >= h || y_sub < 0) {
                    continue;
                }
                int offset = (int)image.channels() * (w * y_sub + x_sub);
                int lvl = (int)((((pixels[offset+0]+pixels[offset+1]+pixels[offset+2]+pixels[offset+3])/4.0)*levels)/65536);
                count[lvl]++;
                avgR[lvl]+=pixels[offset+0];
                avgG[lvl]+=pixels[offset+1];
                avgB[lvl]+=pixels[offset+2];
                avgA[lvl]+=pixels[offset+3];
            }
            
            int cur = 0;
            int idx = 0;
            for(auto i = count.begin(); i != count.end(); i++) {
                if (i->second > cur) {
                    cur = i->second;
                    idx = i->first;
                }
            }
            int finR = (int)avgR[idx]/cur;
            int finG = (int)avgG[idx]/cur;
            int finB = (int)avgB[idx]/cur;
            int finA = (int)avgA[idx]/cur;
            int offset = (int)image.channels() * (w * y + x);
            opixels[offset+0]=finR;
            opixels[offset+1]=finG;
            opixels[offset+2]=finB;
            opixels[offset+3]=finA;
        }
    }
    out.syncPixels();
    Py_END_ALLOW_THREADS
    
    Blob blobout;
    out.write(&blobout);
    return PyBytes_FromStringAndSize(reinterpret_cast<char*>(const_cast<void*>(blobout.data())), blobout.length());
}

static PyMethodDef garbo_methods[] = { 
    {   
        "oil", oil, METH_VARARGS,
        "does a oiling, maybe"
    },  
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef garbo_definition = { 
    PyModuleDef_HEAD_INIT,
    "oil",
    "ehhh",
    -1, 
    garbo_methods
};


PyMODINIT_FUNC PyInit_garbo(void) {
    Py_Initialize();
    return PyModule_Create(&garbo_definition);
}
