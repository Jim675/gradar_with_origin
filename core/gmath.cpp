#include "gmath.h"
#include "gutils.h"

int gRound(double x)
{
	return x > 0 ? (int)(x+0.5) : (int)(x-0.5);
}

int gTrunc(double x)
{
	return (int)x;
}

double gSqr(double x)
{
	return x*x;
}

double gDistance2D( double x0, double y0, double x1, double y1 )
{
	return sqrt( (x1-x0)*(x1-x0)+(y1-y0)*(y1-y0) );
}

void gRotate(double sinAF, double cosAF, double sx, double sy, double &dx, double &dy)
{
	dx = sx*cosAF-sy*sinAF;
	dy = sx*sinAF+sy*cosAF;
}

double gGetAngle(double sinAF, double cosAF)
{
	double af = asin(sinAF);

	if(cosAF < 0) af = PI-af;
	else if(sinAF < 0) af = 2*PI+af;

	return af;
}

bool gFittingLine(double *xArray, double *yArray, int firstIndex, int lastIndex,
				  double &a, double &b)
{
	int count = lastIndex-firstIndex+1;
    if(count < 2) return false;
    double s0 = (double)count, s1 = 0, s2 = 0, t0 = 0, t1 = 0;
    for(int i=firstIndex;i<=lastIndex;i++) 
	{
		s1 += xArray[i];
		s2 += (xArray[i]*xArray[i]);
		t0 += yArray[i];
		t1 += (xArray[i]*yArray[i]);
    }
    double d = s0*s2-s1*s1;
    b = (s2*t0-s1*t1)/d;
    a = (s0*t1-s1*t0)/d;
    return true;
}

double gDeterm3(double a1, double a2, double a3, double b1, double b2, double b3,
				double c1, double c2, double c3)
{
	return a1*(b2*c3-b3*c2) - a2*(b1*c3-b3*c1) + a3*(b1*c2-b2*c1);
}

bool gFittingPlane(double *x, double *y, double *z, int n, double &a, double &b, double &c)
{
	int i;
	double x1, x2, y1, y2, z1, xz, yz, xy, r;

	x1 = x2 = y1 = y2 = z1 = xz = yz = xy = 0;
	for(i=0; i<n; i++)
	{
		x1 += x[i];
		x2 += x[i]*x[i];
		xz += x[i]*z[i];

		y1 += y[i];
		y2 += y[i]*y[i];
		yz += y[i]*z[i];

		z1 += z[i];
		xy += x[i]*y[i];
	}

	r = gDeterm3(x2, xy, x1, xy, y2, y1, x1, y1, n);
	if(IS_ZERO(r)) return false;

	a = gDeterm3(xz, xy, x1, yz, y2, y1, z1, y1, n) / r;
	b = gDeterm3(x2, xz, x1, xy, yz, y1, x1, z1, n) / r;
	c = gDeterm3(x2, xy, xz, xy, y2, yz, x1, y1, z1) / r;
	/*double **mat, *rs;
	gAlloc2(mat, 3, 4);
	gAlloc1(rs, 3);
	mat[0][0] = x2;		mat[0][1] = xy;		mat[0][2] = x1;		mat[0][3] = xz;
	mat[1][0] = xy;		mat[1][1] = y2;		mat[1][2] = y1;		mat[1][3] = yz;
	mat[2][0] = x1;		mat[2][1] = y1;		mat[2][2] = n;		mat[2][3] = z1;
	gSelSlove(mat, rs, 3);
	a = rs[0];	b = rs[1];	c = rs[2];

	gFree2(mat);
	gFree1(rs);*/
	return true;
}

bool gInverseMatrix(double *m, int n)
{
    int i, j, k, l, u, v;
    double d, p;
    
    int *is = new int[n];
    int *js = new int[n];
    
    for(k=0; k<=n-1; k++)
    {
        d = 0;
        for(i=k; i<=n-1; i++)
        {
            for(j=k; j<=n-1; j++)
            {
                l = i*n+j;
                p = fabs(m[l]);
                if(p > d) { d = p;  is[k] = i;  js[k] = j; }
            }
        }
        
        if(IS_ZERO(d))
        {
            delete []is;
            delete []js;
            return false;
        }
        
        if(is[k] != k)
        {
            for(j=0; j<=n-1; j++)
            {
                u = k*n+j;
                v = is[k]*n+j;
                gSwap(m[u], m[v]);
            }
        }
        if(js[k] != k)
        {
            for(i=0; i<=n-1; i++)
            {
                u = i*n+k;
                v = i*n+js[k];
                gSwap(m[u], m[v]);
            }
        }
        
        l = k*n+k;
        m[l] = 1/m[l];
        for(j=0; j<=n-1; j++)
        {
            if(j != k)
            {
                u = k*n+j;
                m[u] = m[u]*m[l];
            }
        }
        
        for(i=0; i<=n-1; i++)
        {
            if(i != k)
            {
                for(j=0; j<=n-1; j++)
                {
                    if(j != k)
                    {
                        u = i*n+j;
                        m[u] = m[u] - m[i*n+k]*m[k*n+j];
                    }
                }
            }
        }
        
        for(i=0; i<=n-1; i++)
        {
            if(i != k)
            {
                u = i*n+k;
                m[u] = -m[u]*m[l];
            }
        }
    }
        
    for(k=n-1; k>=0; k--)
    {
        if(js[k] != k)
        {
            for(j=0; j<=n-1; j++)
            {
                u = k*n+j;
                v = js[k]*n+j;
                gSwap(m[u], m[v]);
            }
        }
        if(is[k] != k)
        {
            for(i=0; i<=n-1; i++)
            {
                u = i*n+k;
                v = i*n+is[k];
                gSwap(m[u], m[v]);
            }
        }
    }
    delete []is;
    delete []js;
    return true;
}

double gUVectorMul(int i, int j, double **matrix)
{
    int k;
	double v=0.0f;
    for(k=0;k<i;k++) v = v+matrix[i][k]*matrix[k][j];
	return v;
}

double gLVectorMul(int i, int j, double **matrix)
{
    int k;
	double v = 0.0f;
	for(k=0; k<j; k++) v = v+matrix[i][k] * matrix[k][j];
	return v;
}

void gSelSlove(double **matrix, double *result, int dim)		//有问题待解决
{
    double *temp, **mat, s1, s2;
	int i, j;
	// 保存行序列
	mat = new double *[dim];
	for(i=0; i<dim; i++) mat[i] = matrix[i];

	for(i=1; i<dim; i++)  //选第一列主元
	{
		if(fabs(matrix[0][0]) < fabs(matrix[i][0]))
		{
			temp = matrix[0];
			matrix[0] = matrix[i];
			matrix[i] = temp;
		}
	}

	for(i=1; i<dim; i++) //算第一列
		matrix[i][0] = matrix[i][0] / matrix[0][0];

	for(i=1; i<dim; i++)
	{
		s1 = matrix[i][i] - gUVectorMul(i, i, matrix);
		for(j=i+1; j<dim; j++)
		{
            s2 = matrix[j][i] - gLVectorMul(j, i, matrix);
			if(fabs(s2) > fabs(s1))
			{
				temp = matrix[i];
				matrix[i] = matrix[j];
				matrix[j] = temp;
				s1 = s2;
			}
		}//选主元
		matrix[i][i] = s1;
		for(j=i+1; j<=dim; j++)
			matrix[i][j] = matrix[i][j] - gUVectorMul(i, j, matrix);
		for(j=i+1; j<dim; j++)
			matrix[j][i] = (matrix[j][i]-gLVectorMul(j, i, matrix)) / matrix[i][i];
	}
	double sum = 0; //余项和
	for(i=dim-1; i>=0; i--)
	{
		for(j=dim-1;j>i;j--) sum += matrix[i][j]*result[j];
		result[i] = (matrix[i][dim]-sum) / matrix[i][i];
		sum = 0;
	}
	// 恢复行序列
	for(i=0; i<dim; i++) matrix[i] = mat[i];
	delete []mat;
}

void gTridagSlove( double *aArray, double *bArray,
					double *cArray, double *dArray,
					int count, double *rArray )
{
	int i;
	for(i=0;i<count-1;i++) 
	{
		aArray[i] = aArray[i]/bArray[i];
		bArray[i+1] = bArray[i+1]-aArray[i]*cArray[i];
		dArray[i+1] = dArray[i+1]-aArray[i]*dArray[i];
	}
	rArray[count-1] = dArray[count-1]/bArray[count-1];
	for(i=count-2;i>=0;i--) 
	{
		rArray[i] =(dArray[i]-cArray[i]*rArray[i+1])/bArray[i];
	}
}

void gTridagExSlove(double aArray[], double bArray[],
				   double cArray[], double dArray[],
				   int count, double rArray[]) 
{
	int i;
	for(i=0;i<count-1;i++) 
	{
		aArray[i+1] = aArray[i+1]/bArray[i];
		bArray[i+1] = bArray[i+1]-aArray[i+1]*cArray[i+1];
		dArray[i+1] = dArray[i+1]-aArray[i+1]*dArray[i];
	}
	bArray[count-1] -= (cArray[count-1] / bArray[0]) * aArray[0];
	dArray[count-1] -= (cArray[count-1] / bArray[0]) * dArray[0];

	rArray[count-1] = dArray[count-1]/bArray[count-1];
	for(i=count-2;i>=0;i--) 
	{
		rArray[i] =(dArray[i]-cArray[i]*rArray[i+1])/bArray[i];
	}

	rArray[0] -= (aArray[0]*rArray[count-1]) / bArray[0];
}

// 解一元二次方程
int gSolveTwoeTimeEqu(double a[3], double r[2])
{
	if(IS_ZERO(a[0])) //一元一次方程
	{
		if(a[1]==0) return 0;
		r[0] = -a[2]/a[1];
		return 1;
	}

	double det = a[1]*a[1] - 4*a[0]*a[2];
	if(det < 0) return 0;

	if(det < GMATH_ZERO)
	{
		r[0] = -a[1]/2/a[0];
		return 1;
	}
	det = sqrt(det);
	r[0] = (-a[1]+det)/2/a[0];
	r[1] = (-a[1]-det)/2/a[0];
	return 2;
}

// 解一元三次方程
int gSolveThreeTimeEqu(double a[4], double r[3])
{
	if(IS_ZERO(a[0]))	//二元一次方程
	{
		return gSolveTwoeTimeEqu(&(a[1]), r);
	}
	for(int i=1; i<4; i++)
	{
		a[i] /= a[0];
	}
	a[0] = 1;

	double p, q;
	p = a[2]-a[1]*a[1]/3;
	q = 2*a[1]*a[1]*a[1]/27 - a[1]*a[2]/3+a[3];

	if(IS_ZERO(p))
	{
		if(q > 0)
		{
			r[0] = -pow(q, 1.0/3.0);
			r[0] -= a[1]/3;
		}
		else
		{
			r[0] = pow(-q, 1.0/3.0);
			r[0] -= a[1]/3;
		}
		return 1;
	}

	if(IS_ZERO(q))
	{
		r[0] = 0;
		r[0] -= a[1]/3;
		if(p < 0)
		{
			r[1] = sqrt(-p);
			r[1] -= a[1]/3;
			return 2;
		}
		return 1;
	}

	double d = q*q/4+p*p*p/27;
	double t = -q/2;
	if(d>0)
	{
		if(t+sqrt(d)>0)
		{
			r[0] = pow(t+sqrt(d),1./3.);
		}
		else
		{
			r[0] = -pow(-t-sqrt(d),1./3.);
		}
		if(t-sqrt(d)>0)
		{
			r[0] += pow(t-sqrt(d),1./3.);
		}
		else
		{
			r[0] -= pow(-t+sqrt(d),1./3.);
		}
		r[0] -= a[1]/3;
		return 1;
	}
	else if(d==0)
	{
		double temp = pow(4*q,1./3.);;
		r[0] = -temp;
		r[0] -= a[1]/3;
		r[1] = temp/2;
		r[1] -= a[1]/3;
		return 2;
	}
	else
	{
		double temp = sqrt(-3*p);
		double arc = acos(-3*q*temp/2/p/p);
		r[0]= (2*temp*cos(arc/3)/3);
		r[0] -= a[1]/3;
		r[1] = (-temp *(cos(arc/3) + sqrt(3.)*sin(arc/3))/3);
		r[1] -= a[1]/3;
		r[2] = (-temp *(cos(arc/3) - sqrt(3.)*sin(arc/3))/3);
		r[2] -= a[1]/3;
		return 3;
	}
}

double gNewtonSolveThreeTimeEqu(double a[4], double r, 
							  double errLimit, int itCount)
{
	int i;
	double x, err;

	err = 1000;
	x = r;
	for(i=0; i<itCount && err > errLimit; i++)
	{
		double f = a[0]*x*x*x + a[1]*x*x + a[2]*x + a[3];
		double df = 3*a[0]*x*x + 2*a[1]*x + a[2];
		if(IS_ZERO(df)) break ;
		x = x - f / df;
		err = fabs(r - x);
		r = x;
	}
	return r;
}

double gRombergInt(double (*func)(double), double a, double b, int nh, double eps)
{
	int i, j, k, m, n;
	double y[10], h, hh, aa, bb, ep, p, x, s, q, r;

	hh = (b-a) / nh;
	aa = a;
	r = 0;
	for(j=0; j<nh; j++)
	{
		bb = aa+hh;		
		y[0] = hh * (func(aa) + func(bb)) * 0.5;
		m = 1;	n = 1;	ep = eps + 1.0;
		h = hh;
		while((ep >= eps) && m <= 9)
		{
			p = 0;
			for(i=0; i<=n-1; i++)
			{
				x = aa + (i+0.5)*h;
				p = p + func(x);
			}
			p = (y[0] + h*p)*0.5;
			s = 1.0;
			for(k=1; k<=m; k++)
			{
				s = 4 * s;
				q = (s*p - y[k-1]) / (s-1.0);
				y[k-1] = p;
				p = q;
			}
			ep = fabs(q - y[m-1]);
			m++;
			y[m-1] = q;
			n += n;
			h *= 0.5;
		}
		aa += hh;
		r += q;
	}
	return r;
}

void trapzInt(double (*func)(double), double x0, double x1, double h, 
				double f0, double f1, double t0, double d, double eps, double t[])
{
	double x, f, t1, t2, p, g, ep;
	x = x0 + h*0.5;
	f = func(x);
	t1 = h*(f0+f)*0.25;
	t2 = h*(f+f1)*0.25;
	p = fabs(t0-(t1+t2));
	//if((p < eps) || (0.5*h < d))
	if(0.5*h < d)
	{
		t[0] = t[0] + (t1+t2);
	}
	else
	{
		g = h*0.5;
		ep = eps/1.4;
		trapzInt(func, x0, x, g, f0, f, t1, d, ep, t);
		trapzInt(func, x, x1, g, f, f1, t2, d, ep, t);
	}
}

double gTrapzInt(double (*func)(double), double a, double b, double h, double eps)
{
	double s, f0, f1, t0, t[2];

	s = b-a;	t[0] = 0.0;
	f0 = func(a);
	f1 = func(b);
	t0 = s*(f0+f1)*0.5;
	trapzInt(func, a, b, s, f0, f1, t0, h, eps, t);
	return t[0];
}

