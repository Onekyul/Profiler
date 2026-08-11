#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(TagName)	ProfileBegin(TagName)
#define PRO_END(TagName)	ProfileEnd(TagName)
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

#include <iostream>
#include <Windows.h>

struct PROFILE_SAMPLE
{
	long			lFlag;				// 사용 여부.
	WCHAR			szName[64];			// 프로파일 이름.

	LARGE_INTEGER	lStartTime;			// 프로파일 실행 시간.

	__int64			iTotalTime;			// 전체 사용시간 
	__int64			iMin[2];			// 최소 사용시간 1,2
	__int64			iMax[2];			// 최대 사용시간 1,2

	__int64			iCall;				// 누적 호출 횟수.

};

PROFILE_SAMPLE g_profile[50];

void ProfileBegin(const WCHAR* szName) {
	LARGE_INTEGER Start;
	LARGE_INTEGER Freq;

	//전역 배열에서 태그를 검색해서 해당 배열이 존재하는지 확인
	int profileIndex = -1;
	for (int i = 0; i < (sizeof(g_profile) / sizeof(PROFILE_SAMPLE)); i++) {
		if (wcscmp(szName, g_profile[i].szName) == 0) profileIndex = i;
	}

	
	 
	// 태그가 배열에 존재하지 않으면 배열에 추가
	if (profileIndex < 0) {
		int addIndex = -1;

		for (int i = 0; i < (sizeof(g_profile) / sizeof(PROFILE_SAMPLE)); i++) {
			if (g_profile[i].lFlag != 1) addIndex = i;
		}

		g_profile[addIndex].lFlag = 1;
		wcscpy_s(g_profile[addIndex].szName, szName);
		g_profile[addIndex].iTotalTime = 0;
		g_profile[addIndex].iMin[0] = MAXLONGLONG;
		g_profile[addIndex].iMin[1] = MAXLONGLONG;
		g_profile[addIndex].iMax[0] = 0;
		g_profile[addIndex].iMax[1] = 0;
		g_profile[addIndex].iCall = 0;

		profileIndex = addIndex;
	}
	 
	
	// 프로파일 시작을 위한 시작 시간 기록
	QueryPerformanceFrequency(&Freq);
	QueryPerformanceCounter(&Start);

	g_profile[profileIndex].lStartTime = Start;
}

void ProfileEnd(const WCHAR* szName) {
	LARGE_INTEGER Freq;
	LARGE_INTEGER Diff;
	LARGE_INTEGER End;
	//전역배열에서 검색해서 존재하는지.
	int profileIndex = -1;
	for (int i = 0; i < (sizeof(g_profile) / sizeof(PROFILE_SAMPLE)); i++) {
		if (wcscmp(szName, g_profile[i].szName) == 0) profileIndex = i;
	}

	if (profileIndex < 0) {
		printf("ProfileEnd: 짝이 맞는 ProfileBegin이 없는 태그입니다.");
		return;
	}

	//프로파일 기록을 위한 종료 시간 기록
	QueryPerformanceFrequency(&Freq);
	QueryPerformanceCounter(&End);
	Diff.QuadPart = End.QuadPart - g_profile[profileIndex].lStartTime.QuadPart;

	// min, max 추가
	if (g_profile[profileIndex].iMin[0] > Diff.QuadPart) {
		g_profile[profileIndex].iMin[1] = g_profile[profileIndex].iMin[0];
		g_profile[profileIndex].iMin[0] = Diff.QuadPart;
	}
	else if (g_profile[profileIndex].iMin[1] > Diff.QuadPart) g_profile[profileIndex].iMin[1] = Diff.QuadPart;

	if (g_profile[profileIndex].iMax[0] < Diff.QuadPart) {
		g_profile[profileIndex].iMax[1] = g_profile[profileIndex].iMax[0];
		g_profile[profileIndex].iMax[0] = Diff.QuadPart;
	}
	else if (g_profile[profileIndex].iMax[1] < Diff.QuadPart) g_profile[profileIndex].iMax[1] = Diff.QuadPart;
	
	//Call 추가 , TotalTime 추가
	g_profile[profileIndex].iCall++;
	g_profile[profileIndex].iTotalTime += Diff.QuadPart;
}

void ProfileDataOutText(const WCHAR* szFileName) {
	//전역 배열 돌면서 출력
	// Average는 구조체의 min,max 배열의 있는 값들을 모두 제외한 뒤 계산.
	// 출력 단위는 마이크로 초 단위로 계산하여 출력.
	FILE* fp = nullptr;
	LARGE_INTEGER Freq;

	QueryPerformanceFrequency(&Freq);
		

	errno_t err = _wfopen_s(&fp, szFileName, L"wt, ccs=UTF-8");
	if (err != 0 || fp == nullptr)
	{
		return;
	}

	fwprintf(fp, L"%16s | %12s | %12s | %12s | %10s\n",	L"Name", L"Average", L"Min", L"Max", L"Call");
	fwprintf(fp, L"---------------------------------------------------------------------------\n");

	for (int i = 0; i < (sizeof(g_profile) / sizeof(PROFILE_SAMPLE)); i++) {
		if (g_profile[i].lFlag != 1) continue;

		PROFILE_SAMPLE& s = g_profile[i];

		__int64 filteredTotalTick = s.iTotalTime - s.iMin[0] - s.iMin[1] - s.iMax[0] - s.iMax[1];
		long long filteredTotalCall = s.iCall - 4;
		double avgUs = (double)filteredTotalTick * 1000000.0 / (double)Freq.QuadPart/ (double)filteredTotalCall;
		double minUs = (double)s.iMin[0] * 1000000.0 / (double)Freq.QuadPart;
		double maxUs = (double)s.iMax[0] * 1000000.0 / (double)Freq.QuadPart;

		fwprintf(fp, L"%16s | %9.4f\u33B2 | %9.4f\u33B2 | %11.4f\u33B2 | %10lld\n",s.szName, avgUs, minUs, maxUs, filteredTotalCall);

	}
	fwprintf(fp, L"---------------------------------------------------------------------------\n");

	fclose(fp);
	
}

void ProfileReset(void) {
	for (int i = 0; i < (sizeof(g_profile) / sizeof(PROFILE_SAMPLE)); i++) {
		g_profile[i].iTotalTime = 0;
		g_profile[i].iMin[0] = MAXLONGLONG;
		g_profile[i].iMin[1] = MAXLONGLONG;
		g_profile[i].iMax[0] = 0;
		g_profile[i].iMax[1] = 0;
		g_profile[i].iCall = 0;
	}

}
