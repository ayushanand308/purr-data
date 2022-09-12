/*
  (c) 2002:cxc@web.fm
  randomix: various PRNG's
  code taken from: http://remus.rutgers.edu/%7Erhoads/Code/code.html
  let's check it out
 */

#include <m_pd.h>
#include <stdlib.h>
#include <math.h>

#define SMALLEST_RANGE .0001

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif

static int makeseed(void);
static int rand_random_icg(int, int);
static unsigned int rand_random_icg_INVERSE_seed(int, int);

static int makeseed(void)
{
    static unsigned int random1_nextseed = 1489853723;
    random1_nextseed = random1_nextseed * 435898247 + 938284287;
    return (random1_nextseed & 0x7fffffff);
}

static int rand_random_icg(int seed, int p)
{
  static int a, b, q, r;
  int state;
  unsigned int inv;
  a = 22211, b = 11926380,q = 96685, r = 12518;
  /* static int p = 2147483053, a = 858993221, b = 1,q = 2, r = 429496611;*/
  /* static int p = 2147483053, a = 579,   b = 24456079, q = 3708951, r = 424;*/
  /* static int p = 2147483053, a = 11972, b = 62187060,q = 179375, r = 5553;*/
  /* static int p = 2147483053, a = 21714, b = 94901263,q = 98898, r = 11881;*/
  /* static int p = 2147483053, a = 4594, b = 44183289,q = 467453, r = 3971;*/
  /* static int p = 2147483647, a = 1288490188, b = 1, q = 1, r = 858993459;*/
  /* static int p = 2147483647, a = 9102, b = 36884165, q = 235935, r =3277;*/
  /* static int p = 2147483647, a = 14288, b = 758634, q = 150299, r = 11535;*/
  /* static int p = 2147483647, a = 21916, b = 71499791, q = 97987, r = 555;*/
  /* static int p = 2147483647, a = 28933, b = 59217914, q = 74222, r = 18521;*/
  /* static int p = 2147483647, a = 31152, b = 48897674, q = 68935, r = 20527;*/

  // state = seed;

  inv = rand_random_icg_INVERSE_seed(seed, p);

  state = a*(inv % q) - r*(inv / q) + b;
  
  if (state < 0) state += p;
  else if (state >= state) state -= p;

  return state;
}

/* Modular Inversion using the extended Euclidean alg. for GCD */
/***************************************************************/
static unsigned rand_random_icg_INVERSE_seed (int state, int p)
{
  unsigned int q,d;
  signed int u,v,inv,t;
  
  if (state <= 1) return(state);
  
  d = p; inv = 0; v = 1; u = state;
  
  do {
    q = d / u;
    t = d % u;
    d = u;
    u = t;
    t = inv - q*v;
    inv = v;
    v = t;
  } while (u != 0);

  if (inv < 0 ) inv += p;

/*   if (1 != d)  */
/*     post ("inverse_iter: Can't invert !"); */

  return(inv);
}

/* -------------------------- random_icg ------------------------------ */
/* inverse congruential generator. */

static t_class *random_icg_tilde_class;

typedef struct _random_icg_tilde
{
  t_object x_obj;
  t_float x_f; // lower limit
  t_float x_g; // upper limit
  t_float x_p; // 1st shared parameter of iter function ..
  unsigned int x_state; // current seed
} t_random_icg_tilde;

static void *random_icg_tilde_new()
{
    t_random_icg_tilde *x = (t_random_icg_tilde *)pd_new(random_icg_tilde_class);
/*     x->x_f = (f) ? f : 0; */
/*     x->x_g = (g) ? g : RAND_MAX; */
    x->x_p = 2147483053;
    //post("cxc/randomix.c: lolim: %f - %f, uplim: %f - %f", x->x_f, f, x->x_g, g);
    x->x_state = makeseed();
    outlet_new(&x->x_obj, gensym("signal"));
    return (x);
}

static void random_icg_tilde_bang(t_random_icg_tilde *x)
{
  double nval;

  x->x_state = rand_random_icg(x->x_state, (int)x->x_p);

  nval = (((x->x_state / x->x_p) - 1) * (double)(x->x_g - x->x_f) + (double)x->x_f);

  // hakc, why is it out of range?
  if(nval < (double)x->x_f) {
    random_icg_tilde_bang(x);
  } else {
    outlet_float(x->x_obj.ob_outlet, nval);
  }
}

static void random_icg_tilde_seed(t_random_icg_tilde *x, float f, float glob)
{
    x->x_state = f;
}

static t_int *random_icg_tilde_perform(t_int *w)
{
  t_random_icg_tilde *x = (t_random_icg_tilde *)(w[1]);
  t_float *out = (t_float *)(w[2]);
  int n = (int)(w[3]);

  while (n--) {
    // generate new noise sample
    x->x_state = rand_random_icg(x->x_state, x->x_p);
    *out++ = (t_float)(x->x_state / x->x_p - 1) * 2 - 1;
  }

  return (w+4);
}


static void random_icg_tilde_dsp(t_random_icg_tilde *x, t_signal **sp)
{
  dsp_add(random_icg_tilde_perform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
}


void random_icg_tilde_setup(void)
{
  random_icg_tilde_class = class_new(gensym("random_icg~"), (t_newmethod)random_icg_tilde_new, 0,
			    sizeof(t_random_icg_tilde), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(random_icg_tilde_class, (t_method)random_icg_tilde_seed,
		  gensym("seed"), A_FLOAT, 0);
  class_addmethod(random_icg_tilde_class, (t_method)random_icg_tilde_dsp, gensym("dsp"), A_CANT, 0);
}
