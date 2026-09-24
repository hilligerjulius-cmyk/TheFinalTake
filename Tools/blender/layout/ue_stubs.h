// Minimal stand-ins for the Unreal types used by the studio/city shell builders.
// Lets Tools/blender/layout/extract_layout.py compile the ORIGINAL Build*() bodies from
// FTStudioShell.cpp / FTCity.cpp with a plain C++ compiler and dump every primitive
// (with its source line) as JSON. Only what those functions touch is implemented.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

typedef int int32;
typedef unsigned int uint32;
typedef unsigned char uint8;
typedef char TCHAR;
#define TEXT(x) x

namespace FMath
{
	inline float Sin(float V) { return std::sin(V); }
	inline float Cos(float V) { return std::cos(V); }
	inline float Frac(float V) { return V - std::floor(V); }
	inline float Abs(float V) { return std::fabs(V); }
	template <class T> inline T Min(T A, T B) { return A < B ? A : B; }
	template <class T> inline T Max(T A, T B) { return A > B ? A : B; }
	template <class T> inline T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }
	inline int32 RoundToInt(float V) { return (int32)std::floor(V + 0.5f); }
	inline int32 FloorToInt(float V) { return (int32)std::floor(V); }
	inline float Sqrt(float V) { return std::sqrt(V); }
}

struct FRotator;

struct FVector
{
	float X = 0.f, Y = 0.f, Z = 0.f;
	FVector() {}
	explicit FVector(float V) : X(V), Y(V), Z(V) {}
	FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
	FVector operator+(const FVector& O) const { return FVector(X + O.X, Y + O.Y, Z + O.Z); }
	FVector operator-(const FVector& O) const { return FVector(X - O.X, Y - O.Y, Z - O.Z); }
	FVector operator-() const { return FVector(-X, -Y, -Z); }
	FVector operator*(float S) const { return FVector(X * S, Y * S, Z * S); }
	FVector operator*(const FVector& O) const { return FVector(X * O.X, Y * O.Y, Z * O.Z); }
	FVector operator/(float S) const { return FVector(X / S, Y / S, Z / S); }
	FVector& operator+=(const FVector& O) { X += O.X; Y += O.Y; Z += O.Z; return *this; }
	FVector GetAbs() const { return FVector(std::fabs(X), std::fabs(Y), std::fabs(Z)); }
	float GetMin() const { return std::min(X, std::min(Y, Z)); }
	float GetMax() const { return std::max(X, std::max(Y, Z)); }
	FRotator Rotation() const;
	static const FVector ZeroVector;
};
inline const FVector FVector::ZeroVector = FVector(0.f, 0.f, 0.f);
inline FVector operator*(float S, const FVector& V) { return V * S; }

struct FRotator
{
	float Pitch = 0.f, Yaw = 0.f, Roll = 0.f;
	FRotator() {}
	FRotator(float P, float Y, float R) : Pitch(P), Yaw(Y), Roll(R) {}
	FRotator operator+(const FRotator& O) const { return FRotator(Pitch + O.Pitch, Yaw + O.Yaw, Roll + O.Roll); }
	/** Same matrix as FRotationMatrix (row-vector convention: out = X*row0 + Y*row1 + Z*row2). */
	FVector RotateVector(const FVector& V) const
	{
		const double D = 3.14159265358979323846 / 180.0;
		const double SP = std::sin(Pitch * D), CP = std::cos(Pitch * D);
		const double SY = std::sin(Yaw * D), CY = std::cos(Yaw * D);
		const double SR = std::sin(Roll * D), CR = std::cos(Roll * D);
		const double M[3][3] = {
			{ CP * CY, CP * SY, SP },
			{ SR * SP * CY - CR * SY, SR * SP * SY + CR * CY, -SR * CP },
			{ -(CR * SP * CY + SR * SY), CY * SR - CR * SP * SY, CR * CP } };
		return FVector(
			(float)(V.X * M[0][0] + V.Y * M[1][0] + V.Z * M[2][0]),
			(float)(V.X * M[0][1] + V.Y * M[1][1] + V.Z * M[2][1]),
			(float)(V.X * M[0][2] + V.Y * M[1][2] + V.Z * M[2][2]));
	}
	FVector Vector() const { return RotateVector(FVector(1.f, 0.f, 0.f)); }
	static const FRotator ZeroRotator;
};
inline const FRotator FRotator::ZeroRotator = FRotator(0.f, 0.f, 0.f);

inline FRotator FVector::Rotation() const
{
	const double R = 180.0 / 3.14159265358979323846;
	return FRotator((float)(std::atan2(Z, std::sqrt(X * X + Y * Y)) * R), (float)(std::atan2(Y, X) * R), 0.f);
}

inline float SrgbToLinear(uint8 C)
{
	const double V = C / 255.0;
	return (float)(V <= 0.04045 ? V / 12.92 : std::pow((V + 0.055) / 1.055, 2.4));
}

struct FColor
{
	uint8 R = 0, G = 0, B = 0, A = 255;
	FColor() {}
	FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : R(InR), G(InG), B(InB), A(InA) {}
	static const FColor White;
};
inline const FColor FColor::White = FColor(255, 255, 255);

struct FLinearColor
{
	float R = 0.f, G = 0.f, B = 0.f, A = 1.f;
	FLinearColor() {}
	FLinearColor(float InR, float InG, float InB, float InA = 1.f) : R(InR), G(InG), B(InB), A(InA) {}
	explicit FLinearColor(const FColor& C) : R(SrgbToLinear(C.R)), G(SrgbToLinear(C.G)), B(SrgbToLinear(C.B)), A(C.A / 255.f) {}
	FLinearColor operator*(float S) const { return FLinearColor(R * S, G * S, B * S, A * S); }
	bool operator==(const FLinearColor& O) const { return R == O.R && G == O.G && B == O.B && A == O.A; }
	FColor ToFColor(bool) const
	{
		auto E = [](float V)
		{
			V = std::min(1.f, std::max(0.f, V));
			const double S = V <= 0.0031308 ? V * 12.92 : 1.055 * std::pow(V, 1.0 / 2.4) - 0.055;
			return (uint8)std::floor(S * 255.999);
		};
		return FColor(E(R), E(G), E(B), (uint8)std::floor(std::min(1.f, std::max(0.f, A)) * 255.999));
	}
	static const FLinearColor White;
};
inline const FLinearColor FLinearColor::White = FLinearColor(1.f, 1.f, 1.f, 1.f);

typedef std::string FString;

// ------------------------------------------------------------------ recording shell

struct FRecord
{
	std::string Kind; // "prim", "text", "point", "spot"
	std::string File;
	int Line = 0;
	std::string Context; // JSON array of enclosing kit calls
	std::string Payload; // JSON object body
};

std::string J(float V);
std::string J(int V);
std::string J(bool V);
std::string J(const FVector& V);
std::string J(const FRotator& V);
std::string J(const FLinearColor& V);
std::string J(const FColor& V);
std::string J(const std::string& V);
std::string J(const char* V);

class Shell
{
public:
	enum EGroup : int32
	{
		CubeSolid, CubeDeco, BoxSolid, BoxDeco, CylSolid, CylDeco, SphereDeco, BallDeco, ConeDeco,
		PrismDeco, RampSolid, TorusDeco, CapsuleDeco, GlassSolid, Blocker, ShorelineDeco, NumGroups
	};
	enum ELightZone : int32 { ZoneExterior = 0, ZoneLobby = 1, ZoneStage = 2, ZoneProjection = 3, ZoneHall = 4 };

	virtual ~Shell() {}
	std::vector<FRecord> Records;
	std::vector<std::string> Stack; // JSON objects of the enclosing kit calls
	std::string CurrentBuild;

	std::string ContextJson() const;
	int32 AddAt(const char* File, int Line, int32 Group, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Emissive = 0.f, const FRotator& Rot = FRotator::ZeroRotator, float Gloss = 0.f);
	void TextAt(const char* File, int Line, const FString& S, const FVector& Loc, float Yaw, float Size, const FColor& Color, float Pitch = 0.f);
	void PointAt(const char* File, int Line, const FVector& Loc, const FLinearColor& Color, float Intensity, float Radius, int32 Zone, bool bShadows = false);
	void SpotAt(const char* File, int Line, const FVector& Loc, const FRotator& Rot, const FLinearColor& Color, float Intensity, float Radius, float Cone, int32 Zone);

	void ArrowImpl(const FVector& From, const FVector& To, const FLinearColor& Color, int32 Count);
	void WallImpl(const FVector& Min, const FVector& Max, const FLinearColor& Color, bool bSolid = true);
	void StairsImpl(const FVector& Start, float Run, float Width, float Rise, int32 Steps, bool bAlongX, const FLinearColor& Tread, const FLinearColor& Edge);
	void PosterImpl(const FVector& Loc, float Yaw, int32 Film, float Scale);
	void PlantImpl(const FVector& Loc, float Scale);
	void CrateImpl(const FVector& Loc, float Size, float Yaw);
	void FlightCaseImpl(const FVector& Loc, const FVector& Size, float Yaw);
	void ConeImpl(const FVector& Loc);
	void PalmImpl(const FVector& Loc, float Height);
	void TrussImpl(float Y, float Z);

	void BuildExterior();
	void BuildLobby();
	void BuildOffice();
	void BuildWardrobe();
	void BuildStage();
	void BuildTankSet();
	void BuildUpperLevel();
	void BuildWarehouse();
};

class CityShell : public Shell
{
public:
	void BuildBoulevard();
	void BuildBlocks();
	void BuildPlaza();
	void BuildCinema();
	void BuildingImpl(float X0, float X1, bool bNorth, float Height, const FLinearColor& Color, const FString& Sign, const FLinearColor& SignColor, int32 Seed);
	void StreetLampImpl(const FVector& Base, float ArmDir, bool bLight);
};

/** Pushes one kit call (name, call site, arguments) for the lifetime of the full expression. */
struct FKitGuard
{
	Shell* S;
	template <class... A>
	FKitGuard(Shell* InS, const char* File, int Line, const char* Name, const A&... Args) : S(InS)
	{
		std::string Json = "{\"kit\":" + J(Name) + ",\"file\":" + J(File) + ",\"line\":" + J(Line) + ",\"args\":[";
		bool bFirst = true;
		((Json += (bFirst ? "" : ","), Json += J(Args), bFirst = false), ...);
		Json += "]}";
		S->Stack.push_back(Json);
	}
	~FKitGuard() { S->Stack.pop_back(); }
};

namespace FTColors
{
	inline FLinearColor Hex(uint32 RGB)
	{
		return FLinearColor(FColor((RGB >> 16) & 0xFF, (RGB >> 8) & 0xFF, RGB & 0xFF, 255));
	}
}

#define Add(...) AddAt(__FILE__, __LINE__, __VA_ARGS__)
#define Text(...) TextAt(__FILE__, __LINE__, __VA_ARGS__)
#define Point(...) PointAt(__FILE__, __LINE__, __VA_ARGS__)
#define Spot(...) SpotAt(__FILE__, __LINE__, __VA_ARGS__)
// arguments are evaluated exactly once (the city passes Seed++)
#define FT_KIT(Name, ...) [&](const auto&... A) { FKitGuard G(this, __FILE__, __LINE__, #Name, A...); Name##Impl(A...); }(__VA_ARGS__)
#define Arrow(...) FT_KIT(Arrow, __VA_ARGS__)
#define Wall(...) FT_KIT(Wall, __VA_ARGS__)
#define Stairs(...) FT_KIT(Stairs, __VA_ARGS__)
#define Poster(...) FT_KIT(Poster, __VA_ARGS__)
#define Plant(...) FT_KIT(Plant, __VA_ARGS__)
#define Crate(...) FT_KIT(Crate, __VA_ARGS__)
#define FlightCase(...) FT_KIT(FlightCase, __VA_ARGS__)
#define Cone(...) FT_KIT(Cone, __VA_ARGS__)
#define Palm(...) FT_KIT(Palm, __VA_ARGS__)
#define Truss(...) FT_KIT(Truss, __VA_ARGS__)
#define Building(...) FT_KIT(Building, __VA_ARGS__)
#define StreetLamp(...) FT_KIT(StreetLamp, __VA_ARGS__)
