#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define SECTION(shm) __attribute__((section(shm))) 
#define cores 3
#define rows 16
#define cols 16
#include <e_lib.h>
 
typedef struct {
    unsigned int row[2];
    unsigned int col[2];
	int flag[3];	
	double q[rows][rows];
	double r[cols][cols];
	int q_flag;
	unsigned long long total_cycles[2];
	
} shm_t;

unsigned int row, col;
unsigned int timer_count;
unsigned long long total_cycles;

volatile shm_t shm SECTION(".shared_dram");

typedef struct {
	int m, n;
	double ** v;
} mat_t, *mat;
 
 
 
 
 void chk_timer_count()
{
    unsigned long long timer_clk;
    
    timer_clk = e_ctimer_get(E_CTIMER_0);
    
    if(timer_clk <= 0)
    {
        timer_count++;
        e_ctimer_set(E_CTIMER_0, E_CTIMER_MAX);
        e_ctimer_start(E_CTIMER_0, E_CTIMER_CLK);
    }
    
}

void init_timer()
{
    timer_count=0;
    e_ctimer_set(E_CTIMER_0, E_CTIMER_MAX);
    e_ctimer_start(E_CTIMER_0, E_CTIMER_CLK);
}

void calc_time()
{
    unsigned long long timer_clk;
    
    timer_clk = E_CTIMER_MAX - e_ctimer_get(E_CTIMER_0);
    
    total_cycles = ((timer_count*E_CTIMER_MAX)+timer_clk);
    
}


mat mat_new(int m, int n)
{
	mat x = malloc(sizeof(mat_t));
	x->v = malloc(sizeof(double*) * m);
	x->v[0] = calloc(sizeof(double), m * n);
	for (int i = 0; i < m; i++)
		x->v[i] = x->v[0] + n * i;
	x->m = m;
	x->n = n;
	return x;
}
 
void mat_del(mat m)
{
	free(m->v[0]);
	free(m->v);
	free(m);
}
 
 
mat mat_cpy(int n, double a[][n], int m)
{
	mat x = mat_new(m, n);
	for (int i = 0; i < m; i++)
		for (int j = 0; j < n; j++)
			x->v[i][j] = a[i][j];
	return x;
}

mat mat_mul_R( mat y,int a,int b)
{
	//if (x->n != y->m) return 0;
	mat r = mat_new(rows, y->n);

	for (int i = a; i < b; i++){
		for (int j = 0; j < y->n; j++){
			for (int k = 0; k < rows; k++){
				r->v[i][j] += shm.q[i][k] * y->v[k][j];
}
}
}
	return r;
}



void delay()

{

for(volatile int i=0;i<1000000;i++ )
	for(volatile int j=0;j<100;j++)
		;

}
 
mat mat_mul(mat x, mat y)
{
	if (x->n != y->m) return 0;
	mat r = mat_new(x->m, y->n);
	for (int i = 0; i < x->m; i++)
		for (int j = 0; j < y->n; j++)
			for (int k = 0; k < x->n; k++)
				r->v[i][j] += x->v[i][k] * y->v[k][j];
	return r;
}
 
mat mat_minor(mat x, int d)
{
	mat m = mat_new(x->m, x->n);
	for (int i = 0; i < d; i++)
		m->v[i][i] = 1;
	for (int i = d; i < x->m; i++)
		for (int j = d; j < x->n; j++)
			m->v[i][j] = x->v[i][j];
	return m;
}
 
/* c = a + b * s */
double *vmadd(double a[], double b[], double s, double c[], int n)
{
	for (int i = 0; i < n; i++)
		c[i] = a[i] + s * b[i];
	return c;
}
 
/* m = I - v v^T */
mat vmul(double v[], int n)
{
	mat x = mat_new(n, n);
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			x->v[i][j] = -2 *  v[i] * v[j];
	for (int i = 0; i < n; i++)
		x->v[i][i] += 1;
 
	return x;
}
 
/* ||x|| */
double mnorm(double x[], int n)
{
	double sum = 0;
	for (int i = 0; i < n; i++) sum += x[i] * x[i];
	return sqrt(sum);
}
 
/* y = x / d */
double* mdiv(double x[], double d, double y[], int n)
{
	for (int i = 0; i < n; i++) y[i] = x[i] / d;
	return y;
}
 
/* take c-th column of m, put in v */
double* mcol(mat m, double *v, int c)
{
	for (int i = 0; i < m->m; i++)
		v[i] = m->v[i][c];
	return v;
}

void matrix_show_Q(mat m)
{
	for(int i = 0; i < m->m; i++) {
		for (int j = 0; j < m->n; j++) {
			shm.q[i][j] =m->v[i][j];
		}
		
	}
	}
 
void matrix_show_R(mat m,int a, int b)
{
	for(int i =a; i < b; i++) {
		for (int j = 0; j < m->n; j++) {
			shm.r[i][j] =m->v[i][j];
		}
		
	}
	}
 
void householder(mat m, mat *R, mat *Q)
{
	mat c[m->m];
	mat z = m, z1;
	for (int k = 0; k < m->n && k < m->m - 1; k++) {
		double e[m->m], x[m->m], a;
		z1 = mat_minor(z, k);
		if (z != m) mat_del(z);
		z = z1;
 
		mcol(z, x, k);
		a = mnorm(x, m->m);
		if (m->v[k][k] > 0) a = -a;
 
		for (int i = 0; i < m->m; i++)
			e[i] = (i == k) ? 1 : 0;
 
		vmadd(x, e, a, e, m->m);
		mdiv(e, mnorm(e, m->m), e, m->m);
		c[k] = vmul(e, m->m);
		z1 = mat_mul(q[k], z);
		if (z != m) mat_del(z);
		z = z1;
	}
	mat_del(z);
	*Q = q[0];
	*R = mat_mul(q[0], m);
	for (int i = 1; i < m->n && i < m->m - 1; i++) {
		z1 = mat_mul(q[i], *Q);
		if (i > 1) mat_del(*Q);
		*Q = z1;
		mat_del(q[i]);
	}
	mat_del(q[0]);
	
	mat_del(*R);
}
 
 
double in[][16] = {
	{5.124575, 6.716550, 4.749593, 0.745557, 0.835284, 1.061532, 2.406306, 3.394185, 7.122869, 4.096635, 7.461806, 5.104174, 0.995246, 3.524249, 1.343407, 7.898948},
	{4.709401, 5.185006, 2.168096, 1.438314, 3.459950, 4.195184, 6.530458, 4.538856, 1.076255, 2.883427, 6.272343, 5.829943, 2.964446, 0.192133, 7.946213, 1.210326},
	{8.183881, 6.291060, 5.034810, 2.737167, 9.811339, 2.984305, 0.413352, 2.290696, 3.698015, 6.639598, 5.670019, 4.373680, 6.214067, 3.592056, 1.280568, 0.145697},
	{6.742811, 6.390177, 2.078934, 8.330168, 6.574254, 0.563682, 4.178736, 4.793037, 9.528616, 8.307987, 2.311288, 6.081988, 3.150382, 9.572844, 0.373962, 3.856355},
	{3.678259, 8.243491, 5.616988, 8.213211, 1.015898, 1.159368, 1.437121, 6.370590, 2.500651, 7.166724, 9.152067, 4.968377, 4.722142, 2.282590, 6.428624, 5.997973},
	{9.094666, 3.641620, 5.956050, 1.629081, 6.459073, 9.468715, 4.752189, 7.788865, 2.220713, 2.484793, 6.529717, 2.083286, 0.599322, 8.827311, 5.588702, 3.405092},
	{5.301290, 2.267589, 0.140771, 1.161778, 0.592439, 9.798719, 4.314055, 3.972274, 9.824009, 9.260374, 6.768367, 5.079985, 5.533086, 1.848284, 3.462898, 0.671444},
	{9.660500, 7.457226, 2.946092, 4.258254, 2.801475, 0.722540, 7.356618, 6.927009, 8.416201, 6.432596, 7.406141, 4.093572, 9.802859, 9.046292, 8.103308, 0.099300},
	{0.001511, 1.054377, 0.355802, 4.236591, 6.461459, 4.018142, 2.400816, 3.651846, 4.161861, 8.746218, 9.976361, 4.284649, 7.141063, 7.410219, 6.350062, 7.652938},
	{4.751442, 8.985795, 9.131959, 0.385422, 6.837505, 9.114349, 0.238815, 8.198015, 8.454410, 2.955473, 7.821795, 0.187684, 6.068312, 3.552302, 2.452192, 3.747190},
	{7.072122, 1.099953, 0.179495, 1.086232, 9.648564, 4.431138, 2.433835, 4.077998, 4.805562, 1.784756, 2.986027, 2.828004, 8.104572, 1.811495, 0.483971, 3.724873},
	{4.902166, 7.299483, 6.822000, 8.115096, 3.138024, 8.902470, 5.816616, 1.066501, 0.368879, 1.762015, 8.535061, 6.834885, 1.670396, 9.097155, 7.376048, 4.097865},
	{0.370360, 3.752868, 6.622229, 5.945147, 8.541925, 8.859654, 3.349494, 2.638748, 1.283802, 9.154308, 8.949241, 9.176734, 9.049492, 8.284714, 4.103983, 9.309067},
	{8.626863, 3.556053, 0.016668, 6.758221, 5.183024, 8.069129, 0.308171, 5.894849, 4.018287, 6.069993, 7.510052, 0.099244, 8.835959, 4.522346, 4.287552, 8.246637},
	{2.015325, 9.879946, 7.386629, 2.522771, 1.881512, 0.924535, 3.546607, 8.241838, 1.206717, 9.042177, 5.350417, 4.953636, 5.543465, 4.732425, 4.703988, 3.690862},
	{0.150104, 1.012173, 6.372757, 4.470197, 8.672633, 9.264992, 5.326238, 1.308067, 6.909482, 6.954018, 5.243287, 0.047543, 5.374593, 4.245235, 9.933439, 8.686286},
};
 
int main()
{
	init_timer();
	e_coords_from_coreid(e_get_coreid(), &row,&col);
	
    shm.flag[row+col]=0;
	shm.flag[3]=0;
	//shm.col[2]=col;
int e[2],d[2];
e[0]=0;
e[1]=(rows/2);
d[0]=(rows);
d[1]=rows;
if(col==0 && row==0){	
mat R, Q;
mat x=mat_cpy(rows ,in ,cols);

R = mat_new(rows,cols);
householder(x,&R,&Q);
matrix_show_Q(Q);
mat z3;
//z3=mat_mul_R(x,0,1);
//matrix_show_R(z3,0,1);
//delay();
delay();
shm.q_flag=1;
}

while(shm.q_flag !=1);

//shm.flag[row+col]=1;
mat c=mat_cpy(rows, in ,cols);

mat z2;
z2=mat_mul_R(c,e[row+col],d[row+col]);
matrix_show_R(z2,e[row+col],d[row+col]);
//matrix_show_R(z2,e[row+col],d[row+col]);
chk_timer_count();
calc_time();

shm.row[row+col]=row;
shm.col[row+col]=col;
shm.flag[row+col]=1;
shm.total_cycles[row+col]=total_cycles;
while(1);
}
