#include "../utils/m_pd.h"
#include <stdlib.h>
#include <Math.h>
#include "../YIN/YIN.h"
#include <string>
#include "../utils/HistogramBuffer.h"
#include "../utils/utils.cpp"

static t_class *yin_class;

typedef struct _yin {
    t_object x_obj;
    t_outlet *pitch_out, *arr_out, *avgOut;
    YIN* yin;
    HistogramBuffer* hb;
} t_yin;

void toFloatArray(int argc, t_atom* argv, float array[]) {
    int i;
    for (i = 0; i < argc; i++) {
        array[i] = atom_getfloat(argv + i);
    }
}

void fromFloatArray(int argc, float* array, t_atom* argv) {
    int i;
    for (i = 0; i < argc; i++) {
        SETFLOAT(argv + i, *(array + i));
    }
}

void yin_list(t_yin *x, t_symbol *s,
        int argc, t_atom *argv) {

    // check list length
    int dim = x->yin->getDimensions();
    if (argc != dim) {
        error("Not correct dimensionality. Should be %d", dim);
        return;
    }

    // copy data into vector and process
    vector<float> data;
    for (int i = 0; i < argc; i++)
        data.push_back(atom_getfloat(argv + i));
    x->yin->process(data);

    int maxdel = x->yin->getMaxDelay();
    t_atom atom_array[maxdel];
//    float* y = x->yin->getYIN();
    fromFloatArray(maxdel, x->yin->getYIN(), atom_array);
    outlet_list(x->arr_out, &s_list, maxdel, atom_array);

//    fromFloatArray(maxdel, x->yin->getAvgs(), atom_array);
//    outlet_list(x->avgOut, &s_list, maxdel, atom_array);
    
    if (x->yin->isSync()) {
        x->hb->put(x->yin->getDip());
        outlet_float(x->pitch_out, (float) x->hb->get());
    } else
        x->hb->drop();
}

void yin_average_threshold(t_yin *x,
        t_floatarg f1) {
    x->yin->setAverageThreshold(f1);
    post("average_threshold set to %f", f1);
}

void yin_dip_threshold(t_yin *x, t_floatarg f1) {
    x->yin->setDipThreshold(f1);
    post("dip_threshold set to %f", f1);
}

static void yin_destructor(t_yin *x) {
    free(x->pitch_out);
    free(x->arr_out);
}

void yin_bufferlength(t_yin *x,
        t_floatarg f1) {
    x->yin->setMaxLength((int) f1);
    post("bufferlength set to %d", (int)f1);
}

void yin_maxdelay(t_yin *x,
        t_floatarg f1) {
    x->yin->setMaxDelay((int) f1);
    post("maxdelay set to %d", (int)f1);
}

void print(t_yin *x,
        t_floatarg f1) {
    post("\n===== YIN =====");
    post("Dimensions = %d", x->yin->getDimensions());
    post("Average threshold = %f", x->yin->getAverageThreshold());
    post("Dip threshold = %f", x->yin->getDipThreshold());
    post("Max delay = %d", x->yin->getMaxDelay());
    post("Max length = %d\n", x->yin->getLength());
}

void *yin_new(t_symbol *s, int argc, t_atom *argv) {
    t_yin *x = (t_yin *) pd_new(
            yin_class);

    int dims = 1;
    if (argc > 0)
        dims = atom_getfloat(argv + 0);

    x->yin = new YIN(dims);
    x->yin->setMinDips(3);
    x->hb = new HistogramBuffer(25);
    
    x->pitch_out = outlet_new(&x->x_obj, &s_float);
    x->arr_out = outlet_new(&x->x_obj, &s_list);
//    x->avgOut = outlet_new(&x->x_obj, &s_list);
    return (void *) x;
}

extern "C"
void yin_setup(void) {

    yin_class = class_new(gensym("yin"),
            (t_newmethod) yin_new, 0,
            sizeof (t_yin), CLASS_DEFAULT, A_GIMME, 0);

    class_addlist(yin_class, yin_list);

    class_addmethod(yin_class,
            (t_method) yin_average_threshold,
            gensym("average_threshold"), A_DEFFLOAT, 0);

    class_addmethod(yin_class,
            (t_method) yin_dip_threshold,
            gensym("dip_threshold"), A_DEFFLOAT, 0);

    class_addmethod(yin_class,
            (t_method) yin_maxdelay,
            gensym("maxdelay"), A_DEFFLOAT, 0);

    class_addmethod(yin_class,
            (t_method) yin_bufferlength,
            gensym("bufferlength"), A_DEFFLOAT, 0);

    class_addmethod(yin_class,
            (t_method) print,
            gensym("print"), A_DEFFLOAT, 0);

    class_sethelpsymbol(yin_class,
            gensym("yin-help"));
}
