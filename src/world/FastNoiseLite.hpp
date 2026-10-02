// FastNoiseLite.hpp
// MIT License
//
// Copyright(c) 2020 Jordan Peck (Auburn)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

#ifndef FNL_DECIMAL
#define FNL_DECIMAL float
#endif

typedef FNL_DECIMAL FNLfloat;

class FastNoiseLite {
public:
    enum NoiseType {
        NoiseType_OpenSimplex2,
        NoiseType_OpenSimplex2S,
        NoiseType_Cellular,
        NoiseType_Perlin,
        NoiseType_ValueCubic,
        NoiseType_Value
    };

    enum FractalType {
        FractalType_None,
        FractalType_FBm,
        FractalType_Ridged,
        FractalType_PingPong,
        FractalType_DomainWarpProgressive,
        FractalType_DomainWarpIndependent
    };

    enum CellularDistanceFunction {
        CellularDistanceFunction_Euclidean,
        CellularDistanceFunction_EuclideanSq,
        CellularDistanceFunction_Manhattan,
        CellularDistanceFunction_Hybrid
    };

    enum CellularReturnType {
        CellularReturnType_CellValue,
        CellularReturnType_Distance,
        CellularReturnType_Distance2,
        CellularReturnType_Distance2Add,
        CellularReturnType_Distance2Sub,
        CellularReturnType_Distance2Mul,
        CellularReturnType_Distance2Div
    };

    enum DomainWarpType {
        DomainWarpType_OpenSimplex2,
        DomainWarpType_OpenSimplex2Reduced,
        DomainWarpType_BasicGrid
    };

    FastNoiseLite(int seed = 1337) {
        SetSeed(seed);
    }

    void SetSeed(int seed) { mSeed = seed; }
    void SetFrequency(FNLfloat frequency) { mFrequency = frequency; }
    void SetNoiseType(NoiseType noiseType) { mNoiseType = noiseType; }
    void SetFractalType(FractalType fractalType) { mFractalType = fractalType; }
    void SetFractalOctaves(int octaves) { mOctaves = octaves; }
    void SetFractalLacunarity(FNLfloat lacunarity) { mLacunarity = lacunarity; }
    void SetFractalGain(FNLfloat gain) { mGain = gain; }
    void SetFractalWeightedStrength(FNLfloat weightedStrength) { mWeightedStrength = weightedStrength; }
    void SetFractalPingPongStrength(FNLfloat pingPongStrength) { mPingPongStrength = pingPongStrength; }
    void SetCellularDistanceFunction(CellularDistanceFunction cellularDistanceFunction) { mCellularDistanceFunction = cellularDistanceFunction; }
    void SetCellularReturnType(CellularReturnType cellularReturnType) { mCellularReturnType = cellularReturnType; }
    void SetCellularJitter(FNLfloat cellularJitter) { mCellularJitterModifier = cellularJitter; }
    void SetDomainWarpType(DomainWarpType domainWarpType) { mDomainWarpType = domainWarpType; }
    void SetDomainWarpAmp(FNLfloat domainWarpAmp) { mDomainWarpAmp = domainWarpAmp; }

    FNLfloat GetNoise(FNLfloat x, FNLfloat y) const {
        TransformNoiseCoordinate2D(x, y);

        switch (mFractalType) {
        default:
            return GenNoiseSingle2D(mSeed, x, y);
        case FractalType_FBm:
            return GenFractalFBm2D(x, y);
        case FractalType_Ridged:
            return GenFractalRidged2D(x, y);
        case FractalType_PingPong:
            return GenFractalPingPong2D(x, y);
        }
    }

    FNLfloat GetNoise(FNLfloat x, FNLfloat y, FNLfloat z) const {
        TransformNoiseCoordinate3D(x, y, z);

        switch (mFractalType) {
        default:
            return GenNoiseSingle3D(mSeed, x, y, z);
        case FractalType_FBm:
            return GenFractalFBm3D(x, y, z);
        case FractalType_Ridged:
            return GenFractalRidged3D(x, y, z);
        case FractalType_PingPong:
            return GenFractalPingPong3D(x, y, z);
        }
    }

    void DomainWarp(FNLfloat& x, FNLfloat& y) const {
        switch (mFractalType) {
        default:
            DomainWarpSingle2D(x, y);
            break;
        case FractalType_DomainWarpProgressive:
            DomainWarpFractalProgressive2D(x, y);
            break;
        case FractalType_DomainWarpIndependent:
            DomainWarpFractalIndependent2D(x, y);
            break;
        }
    }

    void DomainWarp(FNLfloat& x, FNLfloat& y, FNLfloat& z) const {
        switch (mFractalType) {
        default:
            DomainWarpSingle3D(x, y, z);
            break;
        case FractalType_DomainWarpProgressive:
            DomainWarpFractalProgressive3D(x, y, z);
            break;
        case FractalType_DomainWarpIndependent:
            DomainWarpFractalIndependent3D(x, y, z);
            break;
        }
    }

private:
    int mSeed = 1337;
    FNLfloat mFrequency = 0.01f;
    NoiseType mNoiseType = NoiseType_OpenSimplex2;
    FractalType mFractalType = FractalType_None;
    int mOctaves = 3;
    FNLfloat mLacunarity = 2.0f;
    FNLfloat mGain = 0.5f;
    FNLfloat mWeightedStrength = 0.0f;
    FNLfloat mPingPongStrength = 2.0f;
    CellularDistanceFunction mCellularDistanceFunction = CellularDistanceFunction_EuclideanSq;
    CellularReturnType mCellularReturnType = CellularReturnType_Distance;
    FNLfloat mCellularJitterModifier = 1.0f;
    DomainWarpType mDomainWarpType = DomainWarpType_OpenSimplex2;
    FNLfloat mDomainWarpAmp = 1.0f;

    static constexpr FNLfloat SQRT3 = 1.7320508075688772935274463415059f;
    static constexpr FNLfloat F2 = 0.5f * (SQRT3 - 1.0f);
    static constexpr FNLfloat G2 = (3.0f - SQRT3) / 6.0f;
    static constexpr FNLfloat F3 = 1.0f / 3.0f;
    static constexpr FNLfloat G3 = 1.0f / 6.0f;

    static inline int FastFloor(FNLfloat f) {
        return (f >= 0) ? static_cast<int>(f) : static_cast<int>(f - 1.0f);
    }

    static inline FNLfloat InterpHermite(FNLfloat t) {
        return t * t * (3.0f - 2.0f * t);
    }

    static inline FNLfloat InterpQuintic(FNLfloat t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static inline FNLfloat Lerp(FNLfloat a, FNLfloat b, FNLfloat t) {
        return a + t * (b - a);
    }

    static inline int Hash(int seed, int x, int y) {
        int hash = seed ^ (x * 374761393) ^ (y * 668265263);
        hash = (hash ^ (hash >> 13)) * 1274126177;
        return hash ^ (hash >> 16);
    }

    static inline int Hash(int seed, int x, int y, int z) {
        int hash = seed ^ (x * 374761393) ^ (y * 668265263) ^ (z * 1274126177);
        hash = (hash ^ (hash >> 13)) * 1274126177;
        return hash ^ (hash >> 16);
    }

    static inline FNLfloat GradCoord(int seed, int x, int y, FNLfloat xd, FNLfloat yd) {
        int hash = Hash(seed, x, y);
        int h = hash & 7;
        FNLfloat u = h < 4 ? xd : yd;
        FNLfloat v = h < 4 ? yd : xd;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    static inline FNLfloat GradCoord(int seed, int x, int y, int z, FNLfloat xd, FNLfloat yd, FNLfloat zd) {
        int hash = Hash(seed, x, y, z);
        int h = hash & 15;
        FNLfloat u = h < 8 ? xd : yd;
        FNLfloat v = h < 4 ? yd : (h == 12 || h == 14 ? xd : zd);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    void TransformNoiseCoordinate2D(FNLfloat& x, FNLfloat& y) const {
        x *= mFrequency;
        y *= mFrequency;
    }

    void TransformNoiseCoordinate3D(FNLfloat& x, FNLfloat& y, FNLfloat& z) const {
        x *= mFrequency;
        y *= mFrequency;
        z *= mFrequency;
    }

    FNLfloat SinglePerlin2D(int seed, FNLfloat x, FNLfloat y) const {
        int x0 = FastFloor(x);
        int y0 = FastFloor(y);
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        FNLfloat xs = InterpQuintic(x - static_cast<FNLfloat>(x0));
        FNLfloat ys = InterpQuintic(y - static_cast<FNLfloat>(y0));

        FNLfloat xd0 = x - static_cast<FNLfloat>(x0);
        FNLfloat yd0 = y - static_cast<FNLfloat>(y0);
        FNLfloat xd1 = xd0 - 1.0f;
        FNLfloat yd1 = yd0 - 1.0f;

        FNLfloat xf0 = Lerp(GradCoord(seed, x0, y0, xd0, yd0), GradCoord(seed, x1, y0, xd1, yd0), xs);
        FNLfloat xf1 = Lerp(GradCoord(seed, x0, y1, xd0, yd1), GradCoord(seed, x1, y1, xd1, yd1), xs);

        return Lerp(xf0, xf1, ys) * 0.507f;
    }

    FNLfloat SinglePerlin3D(int seed, FNLfloat x, FNLfloat y, FNLfloat z) const {
        int x0 = FastFloor(x);
        int y0 = FastFloor(y);
        int z0 = FastFloor(z);
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        int z1 = z0 + 1;

        FNLfloat xs = InterpQuintic(x - static_cast<FNLfloat>(x0));
        FNLfloat ys = InterpQuintic(y - static_cast<FNLfloat>(y0));
        FNLfloat zs = InterpQuintic(z - static_cast<FNLfloat>(z0));

        FNLfloat xd0 = x - static_cast<FNLfloat>(x0);
        FNLfloat yd0 = y - static_cast<FNLfloat>(y0);
        FNLfloat zd0 = z - static_cast<FNLfloat>(z0);
        FNLfloat xd1 = xd0 - 1.0f;
        FNLfloat yd1 = yd0 - 1.0f;
        FNLfloat zd1 = zd0 - 1.0f;

        FNLfloat xf00 = Lerp(GradCoord(seed, x0, y0, z0, xd0, yd0, zd0), GradCoord(seed, x1, y0, z0, xd1, yd0, zd0), xs);
        FNLfloat xf10 = Lerp(GradCoord(seed, x0, y1, z0, xd0, yd1, zd0), GradCoord(seed, x1, y1, z0, xd1, yd1, zd0), xs);
        FNLfloat xf01 = Lerp(GradCoord(seed, x0, y0, z1, xd0, yd0, zd1), GradCoord(seed, x1, y0, z1, xd1, yd0, zd1), xs);
        FNLfloat xf11 = Lerp(GradCoord(seed, x0, y1, z1, xd0, yd1, zd1), GradCoord(seed, x1, y1, z1, xd1, yd1, zd1), xs);

        FNLfloat yf0 = Lerp(xf00, xf10, ys);
        FNLfloat yf1 = Lerp(xf01, xf11, ys);

        return Lerp(yf0, yf1, zs) * 0.964921414852103f;
    }

    FNLfloat SingleSimplex2D(int seed, FNLfloat x, FNLfloat y) const {
        FNLfloat s = (x + y) * F2;
        int i = FastFloor(x + s);
        int j = FastFloor(y + s);

        FNLfloat t = static_cast<FNLfloat>(i + j) * G2;
        FNLfloat X0 = static_cast<FNLfloat>(i) - t;
        FNLfloat Y0 = static_cast<FNLfloat>(j) - t;
        FNLfloat x0 = x - X0;
        FNLfloat y0 = y - Y0;

        int i1, j1;
        if (x0 > y0) {
            i1 = 1; j1 = 0;
        } else {
            i1 = 0; j1 = 1;
        }

        FNLfloat x1 = x0 - static_cast<FNLfloat>(i1) + G2;
        FNLfloat y1 = y0 - static_cast<FNLfloat>(j1) + G2;
        FNLfloat x2 = x0 - 1.0f + 2.0f * G2;
        FNLfloat y2 = y0 - 1.0f + 2.0f * G2;

        FNLfloat n0, n1, n2;

        FNLfloat t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 < 0.0f) n0 = 0.0f;
        else {
            t0 *= t0;
            n0 = t0 * t0 * GradCoord(seed, i, j, x0, y0);
        }

        FNLfloat t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 < 0.0f) n1 = 0.0f;
        else {
            t1 *= t1;
            n1 = t1 * t1 * GradCoord(seed, i + i1, j + j1, x1, y1);
        }

        FNLfloat t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 < 0.0f) n2 = 0.0f;
        else {
            t2 *= t2;
            n2 = t2 * t2 * GradCoord(seed, i + 1, j + 1, x2, y2);
        }

        return 70.0f * (n0 + n1 + n2);
    }

    FNLfloat SingleCellular2D(int seed, FNLfloat x, FNLfloat y) const {
        int xr = FastFloor(x);
        int yr = FastFloor(y);

        FNLfloat distance[4] = { 1e10f, 1e10f, 1e10f, 1e10f };

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int xi = xr + dx;
                int yi = yr + dy;
                int hash = Hash(seed, xi, yi);
                FNLfloat vx = static_cast<FNLfloat>(xi) + (static_cast<FNLfloat>(hash & 0xFFFF) / 65535.0f) * mCellularJitterModifier;
                FNLfloat vy = static_cast<FNLfloat>(yi) + (static_cast<FNLfloat>((hash >> 16) & 0xFFFF) / 65535.0f) * mCellularJitterModifier;

                FNLfloat dist = (x - vx) * (x - vx) + (y - vy) * (y - vy);
                if (mCellularDistanceFunction == CellularDistanceFunction_Euclidean) dist = std::sqrt(dist);
                else if (mCellularDistanceFunction == CellularDistanceFunction_Manhattan) dist = std::abs(x - vx) + std::abs(y - vy);

                if (dist < distance[0]) {
                    distance[1] = distance[0];
                    distance[0] = dist;
                } else if (dist < distance[1]) {
                    distance[1] = dist;
                }
            }
        }

        switch (mCellularReturnType) {
        case CellularReturnType_CellValue:
            return distance[0];
        case CellularReturnType_Distance:
            return distance[0] - 1.0f;
        case CellularReturnType_Distance2:
            return distance[1] - 1.0f;
        case CellularReturnType_Distance2Add:
            return (distance[1] + distance[0]) * 0.5f - 1.0f;
        case CellularReturnType_Distance2Sub:
            return distance[1] - distance[0] - 1.0f;
        case CellularReturnType_Distance2Mul:
            return distance[1] * distance[0] - 1.0f;
        default:
            return distance[0];
        }
    }

    FNLfloat GenNoiseSingle2D(int seed, FNLfloat x, FNLfloat y) const {
        switch (mNoiseType) {
        case NoiseType_OpenSimplex2:
        case NoiseType_OpenSimplex2S:
            return SingleSimplex2D(seed, x, y);
        case NoiseType_Cellular:
            return SingleCellular2D(seed, x, y);
        case NoiseType_Perlin:
        default:
            return SinglePerlin2D(seed, x, y);
        }
    }

    FNLfloat GenNoiseSingle3D(int seed, FNLfloat x, FNLfloat y, FNLfloat z) const {
        return SinglePerlin3D(seed, x, y, z);
    }

    FNLfloat GenFractalFBm2D(FNLfloat x, FNLfloat y) const {
        FNLfloat sum = 0.0f;
        FNLfloat amp = 1.0f;
        FNLfloat maxAmp = 0.0f;
        int seed = mSeed;

        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat noise = GenNoiseSingle2D(seed++, x, y);
            sum += noise * amp;
            maxAmp += amp;
            amp *= mGain;
            x *= mLacunarity;
            y *= mLacunarity;
        }

        return (maxAmp > 0.0f) ? (sum / maxAmp) : 0.0f;
    }

    FNLfloat GenFractalFBm3D(FNLfloat x, FNLfloat y, FNLfloat z) const {
        FNLfloat sum = 0.0f;
        FNLfloat amp = 1.0f;
        FNLfloat maxAmp = 0.0f;
        int seed = mSeed;

        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat noise = GenNoiseSingle3D(seed++, x, y, z);
            sum += noise * amp;
            maxAmp += amp;
            amp *= mGain;
            x *= mLacunarity;
            y *= mLacunarity;
            z *= mLacunarity;
        }

        return (maxAmp > 0.0f) ? (sum / maxAmp) : 0.0f;
    }

    FNLfloat GenFractalRidged2D(FNLfloat x, FNLfloat y) const {
        FNLfloat sum = 0.0f;
        FNLfloat amp = 1.0f;
        FNLfloat maxAmp = 0.0f;
        int seed = mSeed;

        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat noise = std::abs(GenNoiseSingle2D(seed++, x, y));
            noise = 1.0f - noise;
            sum += noise * amp;
            maxAmp += amp;
            amp *= mGain;
            x *= mLacunarity;
            y *= mLacunarity;
        }

        return (maxAmp > 0.0f) ? (sum / maxAmp) : 0.0f;
    }

    FNLfloat GenFractalRidged3D(FNLfloat x, FNLfloat y, FNLfloat z) const {
        FNLfloat sum = 0.0f;
        FNLfloat amp = 1.0f;
        FNLfloat maxAmp = 0.0f;
        int seed = mSeed;

        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat noise = std::abs(GenNoiseSingle3D(seed++, x, y, z));
            noise = 1.0f - noise;
            sum += noise * amp;
            maxAmp += amp;
            amp *= mGain;
            x *= mLacunarity;
            y *= mLacunarity;
            z *= mLacunarity;
        }

        return (maxAmp > 0.0f) ? (sum / maxAmp) : 0.0f;
    }

    FNLfloat GenFractalPingPong2D(FNLfloat x, FNLfloat y) const {
        FNLfloat sum = 0.0f;
        FNLfloat amp = 1.0f;
        FNLfloat maxAmp = 0.0f;
        int seed = mSeed;

        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat noise = PingPong((GenNoiseSingle2D(seed++, x, y) + 1.0f) * mPingPongStrength);
            sum += (noise - 0.5f) * 2.0f * amp;
            maxAmp += amp;
            amp *= mGain;
            x *= mLacunarity;
            y *= mLacunarity;
        }

        return (maxAmp > 0.0f) ? (sum / maxAmp) : 0.0f;
    }

    FNLfloat GenFractalPingPong3D(FNLfloat x, FNLfloat y, FNLfloat z) const {
        FNLfloat sum = 0.0f;
        FNLfloat amp = 1.0f;
        FNLfloat maxAmp = 0.0f;
        int seed = mSeed;

        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat noise = PingPong((GenNoiseSingle3D(seed++, x, y, z) + 1.0f) * mPingPongStrength);
            sum += (noise - 0.5f) * 2.0f * amp;
            maxAmp += amp;
            amp *= mGain;
            x *= mLacunarity;
            y *= mLacunarity;
            z *= mLacunarity;
        }

        return (maxAmp > 0.0f) ? (sum / maxAmp) : 0.0f;
    }

    static inline FNLfloat PingPong(FNLfloat t) {
        t -= static_cast<FNLfloat>(static_cast<int>(t * 0.5f) * 2);
        return t < 0.0f ? -t : (t > 1.0f ? 2.0f - t : t);
    }

    void DomainWarpSingle2D(FNLfloat& x, FNLfloat& y) const {
        FNLfloat nx = x * mFrequency;
        FNLfloat ny = y * mFrequency;
        FNLfloat qx = SingleSimplex2D(mSeed + 1, nx, ny);
        FNLfloat qy = SingleSimplex2D(mSeed + 2, nx, ny);
        x += qx * mDomainWarpAmp;
        y += qy * mDomainWarpAmp;
    }

    void DomainWarpSingle3D(FNLfloat& x, FNLfloat& y, FNLfloat& z) const {
        FNLfloat nx = x * mFrequency;
        FNLfloat ny = y * mFrequency;
        FNLfloat nz = z * mFrequency;
        FNLfloat qx = SinglePerlin3D(mSeed + 1, nx, ny, nz);
        FNLfloat qy = SinglePerlin3D(mSeed + 2, nx, ny, nz);
        FNLfloat qz = SinglePerlin3D(mSeed + 3, nx, ny, nz);
        x += qx * mDomainWarpAmp;
        y += qy * mDomainWarpAmp;
        z += qz * mDomainWarpAmp;
    }

    void DomainWarpFractalProgressive2D(FNLfloat& x, FNLfloat& y) const {
        FNLfloat amp = mDomainWarpAmp;
        FNLfloat freq = mFrequency;
        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat qx = SingleSimplex2D(mSeed + i * 2, x * freq, y * freq);
            FNLfloat qy = SingleSimplex2D(mSeed + i * 2 + 1, x * freq, y * freq);
            x += qx * amp;
            y += qy * amp;
            amp *= mGain;
            freq *= mLacunarity;
        }
    }

    void DomainWarpFractalProgressive3D(FNLfloat& x, FNLfloat& y, FNLfloat& z) const {
        FNLfloat amp = mDomainWarpAmp;
        FNLfloat freq = mFrequency;
        for (int i = 0; i < mOctaves; ++i) {
            FNLfloat qx = SinglePerlin3D(mSeed + i * 3, x * freq, y * freq, z * freq);
            FNLfloat qy = SinglePerlin3D(mSeed + i * 3 + 1, x * freq, y * freq, z * freq);
            FNLfloat qz = SinglePerlin3D(mSeed + i * 3 + 2, x * freq, y * freq, z * freq);
            x += qx * amp;
            y += qy * amp;
            z += qz * amp;
            amp *= mGain;
            freq *= mLacunarity;
        }
    }

    void DomainWarpFractalIndependent2D(FNLfloat& x, FNLfloat& y) const {
        FNLfloat qx = 0.0f, qy = 0.0f;
        FNLfloat amp = mDomainWarpAmp;
        FNLfloat freq = mFrequency;
        for (int i = 0; i < mOctaves; ++i) {
            qx += SingleSimplex2D(mSeed + i * 2, x * freq, y * freq) * amp;
            qy += SingleSimplex2D(mSeed + i * 2 + 1, x * freq, y * freq) * amp;
            amp *= mGain;
            freq *= mLacunarity;
        }
        x += qx;
        y += qy;
    }

    void DomainWarpFractalIndependent3D(FNLfloat& x, FNLfloat& y, FNLfloat& z) const {
        FNLfloat qx = 0.0f, qy = 0.0f, qz = 0.0f;
        FNLfloat amp = mDomainWarpAmp;
        FNLfloat freq = mFrequency;
        for (int i = 0; i < mOctaves; ++i) {
            qx += SinglePerlin3D(mSeed + i * 3, x * freq, y * freq, z * freq) * amp;
            qy += SinglePerlin3D(mSeed + i * 3 + 1, x * freq, y * freq, z * freq) * amp;
            qz += SinglePerlin3D(mSeed + i * 3 + 2, x * freq, y * freq, z * freq) * amp;
            amp *= mGain;
            freq *= mLacunarity;
        }
        x += qx;
        y += qy;
        z += qz;
    }
};
