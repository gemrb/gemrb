#include <cmath>
#include <SDL.h>
#include "Matrix.h"
#undef near
#undef far

#define I(_i, _j) ((_j)+ 4*(_i))

void Matrix::TransposeM(float* mTrans, float* m)
{
	for (int i = 0; i < 4; i++) 
	{
		int mBase = i * 4;
		mTrans[i] = m[mBase];
		mTrans[i + 4] = m[mBase + 1];
		mTrans[i + 8] = m[mBase + 2];
		mTrans[i + 12] = m[mBase + 3];
	}
}

bool Matrix::InvertM(float* mInv, float* m)
{
	float src0  = m[0];
    float src4  = m[1];
    float src8  = m[2];
    float src12 = m[3];

    float src1  = m[4];
    float src5  = m[5];
    float src9  = m[6];
    float src13 = m[7];

    float src2  = m[8];
    float src6  = m[9];
    float src10 = m[10];
    float src14 = m[11];

    float src3  = m[12];
    float src7  = m[13];
    float src11 = m[14];
    float src15 = m[15];

	// calculate pairs for first 8 elements (cofactors)
    float atmp0  = src10 * src15;
    float atmp1  = src11 * src14;
    float atmp2  = src9  * src15;
    float atmp3  = src11 * src13;
    float atmp4  = src9  * src14;
    float atmp5  = src10 * src13;
    float atmp6  = src8  * src15;
    float atmp7  = src11 * src12;
    float atmp8  = src8  * src14;
    float atmp9  = src10 * src12;
    float atmp10 = src8  * src13;
    float atmp11 = src9  * src12;

    // calculate first 8 elements (cofactors)
    float dst0  = (atmp0 * src5 + atmp3 * src6 + atmp4  * src7) - (atmp1 * src5 + atmp2 * src6 + atmp5  * src7);
    float dst1  = (atmp1 * src4 + atmp6 * src6 + atmp9  * src7) - (atmp0 * src4 + atmp7 * src6 + atmp8  * src7);
    float dst2  = (atmp2 * src4 + atmp7 * src5 + atmp10 * src7) - (atmp3 * src4 + atmp6 * src5 + atmp11 * src7);
    float dst3  = (atmp5 * src4 + atmp8 * src5 + atmp11 * src6) - (atmp4 * src4 + atmp9 * src5 + atmp10 * src6);
    float dst4  = (atmp1 * src1 + atmp2 * src2 + atmp5  * src3) - (atmp0 * src1 + atmp3 * src2 + atmp4  * src3);
    float dst5  = (atmp0 * src0 + atmp7 * src2 + atmp8  * src3) - (atmp1 * src0 + atmp6 * src2 + atmp9  * src3);
    float dst6  = (atmp3 * src0 + atmp6 * src1 + atmp11 * src3) - (atmp2 * src0 + atmp7 * src1 + atmp10 * src3);
    float dst7  = (atmp4 * src0 + atmp9 * src1 + atmp10 * src2) - (atmp5 * src0 + atmp8 * src1 + atmp11 * src2);

    // calculate pairs for second 8 elements (cofactors)
    float btmp0  = src2 * src7;
    float btmp1  = src3 * src6;
    float btmp2  = src1 * src7;
    float btmp3  = src3 * src5;
    float btmp4  = src1 * src6;
    float btmp5  = src2 * src5;
    float btmp6  = src0 * src7;
    float btmp7  = src3 * src4;
    float btmp8  = src0 * src6;
    float btmp9  = src2 * src4;
    float btmp10 = src0 * src5;
    float btmp11 = src1 * src4;

    // calculate second 8 elements (cofactors)
    float dst8  = (btmp0  * src13 + btmp3  * src14 + btmp4  * src15) - (btmp1  * src13 + btmp2  * src14 + btmp5  * src15);
    float dst9  = (btmp1  * src12 + btmp6  * src14 + btmp9  * src15) - (btmp0  * src12 + btmp7  * src14 + btmp8  * src15);
    float dst10 = (btmp2  * src12 + btmp7  * src13 + btmp10 * src15) - (btmp3  * src12 + btmp6  * src13 + btmp11 * src15);
    float dst11 = (btmp5  * src12 + btmp8  * src13 + btmp11 * src14) - (btmp4  * src12 + btmp9  * src13 + btmp10 * src14);
    float dst12 = (btmp2  * src10 + btmp5  * src11 + btmp1  * src9 ) - (btmp4  * src11 + btmp0  * src9  + btmp3  * src10);
    float dst13 = (btmp8  * src11 + btmp0  * src8  + btmp7  * src10) - (btmp6  * src10 + btmp9  * src11 + btmp1  * src8 );
    float dst14 = (btmp6  * src9  + btmp11 * src11 + btmp3  * src8 ) - (btmp10 * src11 + btmp2  * src8  + btmp7  * src9 );
    float dst15 = (btmp10 * src10 + btmp4  * src8  + btmp9  * src9 ) - (btmp8  * src9  + btmp11 * src10 + btmp5  * src8 );

    // calculate determinant
    float det = src0 * dst0 + src1 * dst1 + src2 * dst2 + src3 * dst3;
    if (det == 0.0f) return false;

    // calculate matrix inverse
    float invdet = 1.0f / det;
    mInv[0] = dst0  * invdet;
    mInv[1] = dst1  * invdet;
    mInv[2] = dst2  * invdet;
    mInv[3] = dst3  * invdet;

    mInv[4] = dst4  * invdet;
    mInv[5] = dst5  * invdet;
    mInv[6] = dst6  * invdet;
    mInv[7] = dst7  * invdet;

    mInv[8] = dst8  * invdet;
    mInv[9] = dst9  * invdet;
    mInv[10] = dst10 * invdet;
    mInv[11] = dst11 * invdet;

    mInv[12] = dst12 * invdet;
    mInv[13] = dst13 * invdet;
    mInv[14] = dst14 * invdet;
    mInv[15] = dst15 * invdet;

    return true;
}

void Matrix::OrthoM(float* m, float left, float right, float bottom, float top, float near, float far) 
{
    if (left == right) return;
    if (bottom == top) return;
    if (near == far) return;

    float r_width  = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth  = 1.0f / (far - near);
    float x =  2.0f * (r_width);
    float y =  2.0f * (r_height);
    float z = -2.0f * (r_depth);
    float tx = -(right + left) * r_width;
    float ty = -(top + bottom) * r_height;
    float tz = -(far + near) * r_depth;
    m[0] = x;
    m[5] = y;
    m[10] = z;
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
    m[15] = 1.0f;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[11] = 0.0f;
}

void Matrix::FrustumM(float* m, float left, float right, float bottom, float top, float near, float far) 
{
    if (left == right) return;
    if (top == bottom) return;
    if (near == far) return;
    if (near <= 0.0f) return;
    if (far <= 0.0f) return;
    float r_width  = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth  = 1.0f / (near - far);
    float x = 2.0f * (near * r_width);
    float y = 2.0f * (near * r_height);
    float A = (right + left) * r_width;
    float B = (top + bottom) * r_height;
    float C = (far + near) * r_depth;
    float D = 2.0f * (far * near * r_depth);
    m[0] = x;
    m[5] = y;
    m[8] = A;
    m[9] = B;
    m[10] = C;
    m[14] = D;
    m[11] = -1.0f;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[15] = 0.0f;
}

void Matrix::PerspectiveM(float* m, float fovy, float aspect, float zNear, float zFar)
{
    float f = 1.0f / (float) tanf(fovy * (M_PI / 360.0));
    float rangeReciprocal = 1.0f / (zNear - zFar);

    m[0] = f / aspect;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;

    m[4] = 0.0f;
    m[5] = f;
    m[6] = 0.0f;
    m[7] = 0.0f;

    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = (zFar + zNear) * rangeReciprocal;
    m[11] = -1.0f;

    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 2.0f * zFar * zNear * rangeReciprocal;
    m[15] = 0.0f;
}

float Matrix::Length(float x, float y, float z)
{
    return (float) sqrtf(x * x + y * y + z * z);
}

void Matrix::SetIdentityM(float* sm)
{
    for (int i = 0 ; i < 16 ; i++) 
	{
        sm[i] = 0;
    }
    for(int i = 0; i < 16; i += 5) 
	{
        sm[i] = 1.0f;
    }
}

void Matrix::ScaleM(float* sm, float* m, float x, float y, float z)
{
    for (int i = 0 ; i < 4 ; i++) 
	{
        sm[     i] = m[     i] * x;
        sm[ 4 + i] = m[ 4 + i] * y;
        sm[ 8 + i] = m[ 8 + i] * z;
        sm[12 + i] = m[12 + i];
    }
}

void Matrix::ScaleM(float* m, float x, float y, float z) 
{
    for (int i = 0 ; i < 4 ; i++) 
	{
        m[     i] *= x;
        m[ 4 + i] *= y;
        m[ 8 + i] *= z;
    }
}

void Matrix::TranslateM(float* tm, float* m, float x, float y, float z)
{
    for (int i = 0 ; i < 12 ; i++) 
	{
        tm[i] = m[i];
    }
    for (int i = 0 ; i < 4 ; i++) 
	{
        tm[12 + i] = m[i] * x + m[4 + i] * y + m[8 + i] * z + m[12 + i];
    }
}

void Matrix::TranslateM(float* m, float x, float y, float z)
{
    for (int i = 0 ; i < 4 ; i++) 
	{
        m[12 + i] += m[i] * x + m[4 + i] * y + m[8 + i] * z;
    }
}

void Matrix::SetRotateM(float* rm, float a, float x, float y, float z)
{
    rm[3] = 0;
    rm[7] = 0;
    rm[11]= 0;
    rm[12]= 0;
    rm[13]= 0;
    rm[14]= 0;
    rm[15]= 1;
    a *= (float) (M_PI / 180.0f);
    float s = (float) sinf(a);
    float c = (float) cosf(a);
    if (1.0f == x && 0.0f == y && 0.0f == z)
	{
        rm[5] = c;   rm[10]= c;
        rm[6] = s;   rm[9] = -s;
        rm[1] = 0;   rm[2] = 0;
        rm[4] = 0;   rm[8] = 0;
        rm[0] = 1;
    } 
	else if (0.0f == x && 1.0f == y && 0.0f == z) 
	{
        rm[0] = c;   rm[10]= c;
        rm[8] = s;   rm[2] = -s;
        rm[1] = 0;   rm[4] = 0;
        rm[6] = 0;   rm[9] = 0;
        rm[5] = 1;
    } 
	else if (0.0f == x && 0.0f == y && 1.0f == z) 
	{
        rm[0] = c;   rm[5] = c;
        rm[1] = s;   rm[4] = -s;
        rm[2] = 0;   rm[6] = 0;
        rm[8] = 0;   rm[9] = 0;
        rm[10]= 1;
    } 
	else 
	{
        float len = Length(x, y, z);
        if (1.0f != len) 
		{
            float recipLen = 1.0f / len;
            x *= recipLen;
            y *= recipLen;
            z *= recipLen;
        }
        float nc = 1.0f - c;
        float xy = x * y;
        float yz = y * z;
        float zx = z * x;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        rm[0] = x*x*nc +  c;
        rm[4] =  xy*nc - zs;
        rm[8] =  zx*nc + ys;
        rm[1] =  xy*nc + zs;
        rm[5] = y*y*nc +  c;
        rm[9] =  yz*nc - xs;
        rm[2] =  zx*nc - ys;
        rm[6] =  yz*nc + xs;
        rm[10] = z*z*nc +  c;
    }
}

void Matrix::RotateM(float* rm, float* m, float a, float x, float y, float z)
{
	float sTemp[32];
    SetRotateM(sTemp, a, x, y, z);
    MultiplyMM(rm, m, sTemp);
}

void Matrix::RotateM(float* m, float a, float x, float y, float z) 
{
	float sTemp[32];
    SetRotateM(sTemp, a, x, y, z);
    MultiplyMM(&sTemp[16], m, sTemp);
    memcpy(m, &sTemp[16], 16);
}

void Matrix::SetRotateEulerM(float* rm, float x, float y, float z)
{
    x *= (float) (M_PI / 180.0f);
    y *= (float) (M_PI / 180.0f);
    z *= (float) (M_PI / 180.0f);
    float cx = cosf(x);
    float sx = sinf(x);
    float cy = cosf(y);
    float sy = sinf(y);
    float cz = cosf(z);
    float sz = sinf(z);
    float cxsy = cx * sy;
    float sxsy = sx * sy;

    rm[0]  =   cy * cz;
    rm[1]  =  -cy * sz;
    rm[2]  =   sy;
    rm[3]  =  0.0f;

    rm[4]  =  cxsy * cz + cx * sz;
    rm[5]  = -cxsy * sz + cx * cz;
    rm[6]  =  -sx * cy;
    rm[7]  =  0.0f;

    rm[8]  = -sxsy * cz + sx * sz;
    rm[9]  =  sxsy * sz + sx * cz;
    rm[10] =  cx * cy;
    rm[11] =  0.0f;

    rm[12] =  0.0f;
    rm[13] =  0.0f;
    rm[14] =  0.0f;
    rm[15] =  1.0f;
}

void Matrix::MultiplyMM(float* result, const float* lhs, const float* rhs)
{
	for (int i = 0; i < 4; i++) 
	{
        float rhs_i0 = rhs[ I(i,0) ];
        float ri0 = lhs[ I(0,0) ] * rhs_i0;
        float ri1 = lhs[ I(0,1) ] * rhs_i0;
        float ri2 = lhs[ I(0,2) ] * rhs_i0;
        float ri3 = lhs[ I(0,3) ] * rhs_i0;
        for (int j = 1; j < 4; j++) 
		{
            float rhs_ij = rhs[ I(i,j) ];
            ri0 += lhs[ I(j,0) ] * rhs_ij;
            ri1 += lhs[ I(j,1) ] * rhs_ij;
            ri2 += lhs[ I(j,2) ] * rhs_ij;
            ri3 += lhs[ I(j,3) ] * rhs_ij;
        }
        result[ I(i,0) ] = ri0;
        result[ I(i,1) ] = ri1;
        result[ I(i,2) ] = ri2;
        result[ I(i,3) ] = ri3;
    }
}

void Matrix::MultiplyMV(float* result, const float* lhs, const float* rhs)
{
    mx4transform(rhs[0], rhs[1], rhs[2], rhs[3], lhs, result);
}

void Matrix::SetLookAtM(float* rm, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, 
				float centerZ, float upX, float upY, float upZ)
{
    // See the OpenGL GLUT documentation for gluLookAt for a description
    // of the algorithm. We implement it in a straightforward way:

    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize f
    float rlf = 1.0f / Length(fx, fy, fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // compute s = f x up (x means "cross product")
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    // and normalize s
    float rls = 1.0f / Length(sx, sy, sz);
    sx *= rls;
    sy *= rls;
    sz *= rls;

    // compute u = s x f
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    rm[0] = sx;
    rm[1] = ux;
    rm[2] = -fx;
    rm[3] = 0.0f;

    rm[4] = sy;
    rm[5] = uy;
    rm[6] = -fy;
    rm[7] = 0.0f;

    rm[8] = sz;
    rm[9] = uz;
    rm[10] = -fz;
    rm[11] = 0.0f;

    rm[12] = 0.0f;
    rm[13] = 0.0f;
    rm[14] = 0.0f;
    rm[15] = 1.0f;

    TranslateM(rm, -eyeX, -eyeY, -eyeZ);
}
