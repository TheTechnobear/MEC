
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_VECTOR__
#define __ML_VECTOR__

// vector classes for small objects with fixed sizes, where the overhead
// of MLSignals would be too big. 

#include "MLDSP.h"

#if defined(ML_USE_SSE)
    #include "source/sse/MLVector.h"
#elif defined(ML_USE_NEON)
    #include "source/neon/MLVector.h"
#else 
    #include "source/nofpu/MLVector.h"
#endif


class Vec2 : public MLVec 
{
public:
	Vec2() : MLVec() {}
	Vec2(const MLVec& b) : MLVec(b) {};
	Vec2(float px, float py) { val.f[0] = px; val.f[1] = py; val.f[2] = 0.; val.f[3] = 0.; }
	float x() const { return val.f[0]; }
	float y() const { return val.f[1]; }
	void setX(float f) { val.f[0] = f; }
	void setY(float f) { val.f[1] = f; }
	float magnitude() const;
};

class Vec3 : public MLVec 
{
public:
	Vec3() : MLVec() {}
	Vec3(const MLVec& b) : MLVec(b) {};
	Vec3(float px, float py, float pz) { val.f[0] = px; val.f[1] = py; val.f[2] = pz; val.f[3] = 0.; }
	float x() const { return val.f[0]; }
	float y() const { return val.f[1]; }
	float z() const { return val.f[2]; }
	void setX(float f) { val.f[0] = f; }
	void setY(float f) { val.f[1] = f; }
	void setZ(float f) { val.f[2] = f; }
	float magnitude() const;
};

class Vec4 : public MLVec 
{
public:
	Vec4() : MLVec() {}
	Vec4(const MLVec& b) : MLVec(b) {};
	Vec4(float px, float py, float pz, float pw) { val.f[0] = px; val.f[1] = py; val.f[2] = pz; val.f[3] = pw; }
	float x() const { return val.f[0]; }
	float y() const { return val.f[1]; }
	float z() const { return val.f[2]; }
	float w() const { return val.f[3]; }
	void setX(float f) { val.f[0] = f; }
	void setY(float f) { val.f[1] = f; }
	void setZ(float f) { val.f[2] = f; }
	void setW(float f) { val.f[3] = f; }
	float magnitude() const;
};

// rectangle stored in left / top / width / height format.
class MLRect : public MLVec 
{
public:
	MLRect() : MLVec() {}
	MLRect(const MLVec& b) : MLVec(b) {};
	MLRect(float x, float y, float width, float height); 
	MLRect(const Vec2& corner1, const Vec2& corner2);
    float left() const { return val.f[0]; }
	float top() const { return val.f[1]; }	
	float right() const { return val.f[0] + val.f[2]; }
	float bottom() const { return val.f[1] + val.f[3]; }	
	float width() const { return val.f[2]; }
	float height() const { return val.f[3]; }
	
	inline float area() const { return width()*height(); }
	inline bool contains(const Vec2& p) const { return (within(p.x(), left(), right()) && within(p.y(), top(), bottom())); }
	MLRect intersect(const MLRect& b) const;
	MLRect unionWith(const MLRect& b) const;
	bool intersects(const MLRect& p) const;

	void setToIntersectionWith(const MLRect& b); 
	void setToUnionWith(const MLRect& b); 
	
	inline void setOrigin(Vec2 b){ val.f[0] = b.val.f[0]; val.f[1] = b.val.f[1]; }
	inline void setLeft(float px){ val.f[0] = px; }
	inline void setTop(float py){ val.f[1] = py; }
	inline void setWidth(float w){ val.f[2] = w; }
	inline void setHeight(float h){ val.f[3] = h; }
	
	inline void setRight(float px){ val.f[0] = px - val.f[2]; }
	inline void setBottom(float py){ val.f[1] = py - val.f[3]; }
	void translate(const Vec2& b);
	void setCenter(const Vec2& b);
	void centerInRect(const MLRect& b);

	inline void stretchWidth(float d){ val.f[0] -= d*0.5f; val.f[2] += d; }
	inline void stretchHeight(float d){ val.f[1] -= d*0.5f; val.f[3] += d; }
	
	inline void stretchWidthTo(float w){ float d = w - width(); stretchWidth(d); }
	inline void stretchHeightTo(float h){ float d = h - height(); stretchHeight(d); }	
	
	inline void expand(float d){ stretchWidth(d); stretchHeight(d); }
	inline void expand(const Vec2& b){ stretchWidth(b.x()); stretchHeight(b.y()); }
	inline void shrink(float d){ expand(-d); }
	inline void shrink(const Vec2& b){ expand(-b); }
	
	void quantize(int q);
	
	MLRect translated(const Vec2& b) const;
	MLRect withCenter(const Vec2& b) const;
	MLRect withCenter(const float cx, const float cy);
	MLRect withTopLeft(const Vec2& b) const;
	MLRect withTopLeft(const float cx, const float cy);

	Vec2 getCenter() const;
	Vec2 getTopLeft() const;
	Vec2 getBottomRight() const;
	
	// adapters for JUCE drawing (TODO remove)
	inline bool contains(int px, int py) const { return (within(px, (int)left(), (int)right()) && within(py, (int)top(), (int)bottom())); }
	inline void setBounds(int l, int t, int w, int h) { *this = MLRect((float)l, (float)t, (float)w, (float)h); }
	inline int x() const { return (int)left(); }
	inline int y() const { return (int)top(); }
	inline int getWidth() const { return (int)width(); }
	inline int getHeight() const { return (int)height(); }
};

std::ostream& operator<< (std::ostream& out, const Vec2& r);
std::ostream& operator<< (std::ostream& out, const Vec3& r);
std::ostream& operator<< (std::ostream& out, const Vec4& r);
std::ostream& operator<< (std::ostream& out, const MLRect& r);

#define MLPoint Vec2

#endif // __ML_VECTOR__

