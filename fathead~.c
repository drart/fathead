#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <strings.h>
#include <string.h>

#include "m_pd.h"
#define t_floatarg float
#define atom_getsymarg atom_getsymbolarg

static t_class *fathead_class;

#define OBJECT_NAME "fathead~"
#define FATHEAD_MSG "by Adam Tindale"
#ifndef PIOVERTWO
#define PIOVERTWO 1.5707963268
#endif
#ifndef TWOPI
#define TWOPI 6.2831853072
#endif
#ifndef PI
#define PI 3.14159265358979
#endif

typedef struct _fathead
{
  t_object x_obj;
  float x_f;
  float *gbuf;
  long grainsamps;
  long buflen ; // length of buffer
  float *grainenv; 
  long gpt1; // grain pointer 1
  long gpt2; // grain pointer 2
  long gpt3; // grain pointer 3
  float phs1; // phase 1
  float phs2; // phase 2
  float phs3; // phase 3
  float incr;
  long curdel;
  short mute_me;
  short iconnect;
  // float *fhbuffer;

} t_fathead;



void *fathead_new(t_floatarg val);
t_int *offset_perform(t_int *w);
t_int *fathead_perform(t_int *w);
void fathead_dsp(t_fathead *x, t_signal **sp, short *count);
void fathead_assist(t_fathead *x, void *b, long m, long a, char *s);
void fathead_mute(t_fathead *x, t_floatarg toggle);
void fathead_float(t_fathead *x, double f ) ;
void fathead_dsp_free(t_fathead *x);

void fathead_tilde_setup(void){
  fathead_class = class_new(gensym("fathead~"), (t_newmethod)fathead_new, 
			    (t_method)fathead_dsp_free ,sizeof(t_fathead), 0,A_FLOAT,0);
  CLASS_MAINSIGNALIN(fathead_class, t_fathead, x_f);
  class_addmethod(fathead_class,(t_method)fathead_dsp,gensym("dsp"),0);
  class_addmethod(fathead_class,(t_method)fathead_mute,gensym("mute"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME, FATHEAD_MSG);
}

void fathead_float(t_fathead *x, double f) {
  x->incr = f; 
} 

void fathead_dsp_free(t_fathead *x)
{
  free(x->gbuf);
  free(x->grainenv);
}


void fathead_mute(t_fathead *x, t_floatarg toggle)
{
  x->mute_me = (short)toggle;
}

void fathead_assist (t_fathead *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:sprintf(dst,"(signal) Input");break;
    case 1:sprintf(dst,"(signal/float) Increment");break;
    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }
}

void *fathead_new(t_floatarg val)
{
  int i;
  t_fathead *x = (t_fathead *)pd_new(fathead_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));

  // INITIALIZATIONS
  if( val > 0 ) {
    x->grainsamps = val;
//    post( "grainsize set to %.0f", val );
  } else {
    x->grainsamps = 2048;
    // post( "grainsize defaults to %d, val was %.0f", x->grainsamps, val );

  }
  x->buflen = x->grainsamps * 4;
  x->gbuf = (float *) calloc( x->buflen, sizeof(float) ) ;
  x->grainenv = (float *) calloc( x->grainsamps, sizeof(float) );
  for(i = 0; i < x->grainsamps; i++ ){
    x->grainenv[i] = .5 + (-.5 * cos( TWOPI * ((float)i/(float)x->grainsamps) ) );
  }
  x->gpt1 = 0;
  x->gpt2 = x->grainsamps / 3.;
  x->gpt3 = 2. * x->grainsamps / 3.;
  x->phs1 = 0;
  x->phs2 = x->grainsamps / 3. ;
  x->phs3 = 2. * x->grainsamps / 3. ;
  x->incr = .5 ;
  x->curdel = 0;
  x->mute_me = 0;

  return (x);
}

t_int *fathead_perform(t_int *w)
{
	float  outsamp ;
	int iphs_a, iphs_b;
	float frac;

	
	/****/
	t_fathead *x = (t_fathead *) (w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *increment = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int)(w[5]);
	int iconnect = x->iconnect;
	
	long gpt1 = x->gpt1;
	long gpt2 = x->gpt2;
	long gpt3 = x->gpt3;
	float phs1 = x->phs1;
	float phs2 = x->phs2;
	float phs3 = x->phs3;
	long curdel = x->curdel;
	long buflen = x->buflen;
	long grainsamps = x->grainsamps;
	float *grainenv = x->grainenv;
	float *gbuf = x->gbuf;
	float incr = x->incr;
	
	if( x->mute_me ) {
		while( n-- ){
			*out++ = 0.0;
		}
		return (w+6);
	} 
	
	while (n--) { 
		x->incr = *increment++;
		
		if( x->incr <= 0. ) {
			x->incr = .5 ;
		}
		
		if( curdel >= buflen ){
			curdel = 0 ;
		}    
		gbuf[ curdel ] = *in++;
    	
		// grain 1 
		iphs_a = floor( phs1 );
		iphs_b = iphs_a + 1;
		
		frac = phs1 - iphs_a;
		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp = (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt1++ ];

		if( gpt1 >= grainsamps ) {
			
			gpt1 = 0;
			phs1 = curdel;
		}
		phs1 += incr;
		while( phs1 >= buflen ) {
			phs1 -= buflen;
		}
		
		// now add second grain 
		
		
		iphs_a = floor( phs2 );
		
		iphs_b = iphs_a + 1;
		
		frac = phs2 - iphs_a;

		
		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp += (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt2++ ];
		if( gpt2 >= grainsamps ) {
			gpt2 = 0;
			phs2 = curdel ;
		}
		phs2 += incr ;    
		while( phs2 >= buflen ) {
			phs2 -= buflen ;
		}
		
		// now add third grain 
		
		iphs_a = floor( phs3 );
		iphs_b = iphs_a + 1;
		
		frac = phs3 - iphs_a;

		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp += (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt3++ ];
		if( gpt3 >= grainsamps ) {
			gpt3 = 0;
			phs3 = curdel ;
		}
		phs3 += incr ;    
		while( phs3 >= buflen ) {
			phs3 -= buflen ;
		}
		
		
		++curdel;
		
		*out++ = outsamp; 
		/* output may well need to attenuated */
	}
	x->phs1 = phs1;
	x->phs2 = phs2;
	x->phs3 = phs3;
	x->gpt1 = gpt1;
	x->gpt2 = gpt2;
	x->gpt3 = gpt3;
	x->curdel = curdel;
	return (w+6);
	
}		

void fathead_dsp(t_fathead *x, t_signal **sp, short *count)
{
  dsp_add(fathead_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  sp[0]->s_n);
}

