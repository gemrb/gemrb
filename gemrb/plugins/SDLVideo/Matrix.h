class Matrix
{
public:
	static void MultiplyMV(float* result, const float* lhs, const float* rhs);
	static void MultiplyMM(float* result, const float* lhs, const float* rhs);

	static void TransposeM(float* mTrans, float* m);
	static bool InvertM(float* mInv, float* m);
	static void OrthoM(float* m, float left, float right, float bottom, float top, float near, float far);
	static void FrustumM(float* m, float left, float right, float bottom, float top, float near, float far);
	static void PerspectiveM(float* m, float fovy, float aspect, float zNear, float zFar);
	static float Length(float x, float y, float z);
	static void SetIdentityM(float* sm);
	static void ScaleM(float* sm, float* m, float x, float y, float z);
	static void ScaleM(float* m, float x, float y, float z);
	static void TranslateM(float* tm, float* m, float x, float y, float z);
	static void TranslateM(float* m, float x, float y, float z);
	static void SetRotateM(float* rm, float a, float x, float y, float z);
	static void RotateM(float* rm, float* m, float a, float x, float y, float z);
	static void RotateM(float* m, float a, float x, float y, float z);
	static void SetRotateEulerM(float* rm, float x, float y, float z);
	static void SetLookAtM(float* rm, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, 
				float centerZ, float upX, float upY, float upZ);

private:
	static inline void mx4transform(float x, float y, float z, float w, const float* pM, float* pDest) 
	{
		pDest[0] = pM[0 + 4 * 0] * x + pM[0 + 4 * 1] * y + pM[0 + 4 * 2] * z + pM[0 + 4 * 3] * w;
		pDest[1] = pM[1 + 4 * 0] * x + pM[1 + 4 * 1] * y + pM[1 + 4 * 2] * z + pM[1 + 4 * 3] * w;
		pDest[2] = pM[2 + 4 * 0] * x + pM[2 + 4 * 1] * y + pM[2 + 4 * 2] * z + pM[2 + 4 * 3] * w;
		pDest[3] = pM[3 + 4 * 0] * x + pM[3 + 4 * 1] * y + pM[3 + 4 * 2] * z + pM[3 + 4 * 3] * w;
	}
};
