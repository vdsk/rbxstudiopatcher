// grpatcher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <Windows.h>
#include <intrin.h>
#include <vector>
#include <sstream>


DWORD dwFindPattern(unsigned char* pData, unsigned int length, const unsigned char* pat, const char* msk)
{
    const unsigned char* end = pData + length - strlen(msk);
    int num_masks = ceil((float)strlen(msk) / (float)16);
    int masks[32]; //32*16 = enough masks for 512 bytes
    memset(masks, 0, num_masks * sizeof(int));
    for (int i = 0; i < num_masks; ++i)
        for (int j = strnlen(msk + i * 16, 16) - 1; j >= 0; --j)
            if (msk[i * 16 + j] == 'x')
                masks[i] |= 1 << j;

    __m128i xmm1 = _mm_loadu_si128((const __m128i*) pat);
    __m128i xmm2, xmm3, mask;
    for (; pData != end; _mm_prefetch((const char*)(++pData + 64), _MM_HINT_NTA)) {
        if (pat[0] == pData[0]) {
            xmm2 = _mm_loadu_si128((const __m128i*) pData);
            mask = _mm_cmpeq_epi8(xmm1, xmm2);
            if ((_mm_movemask_epi8(mask) & masks[0]) == masks[0]) {
                for (int i = 1; i < num_masks; ++i) {
                    xmm2 = _mm_loadu_si128((const __m128i*) (pData + i * 16));
                    xmm3 = _mm_loadu_si128((const __m128i*) (pat + i * 16));
                    mask = _mm_cmpeq_epi8(xmm2, xmm3);
                    if ((_mm_movemask_epi8(mask) & masks[i]) == masks[i]) {
                        if ((i + 1) == num_masks)
                            return (DWORD)pData;
                    }
                    else goto cont;
                }
                return (DWORD)pData;
            }
        }cont:;
    }
    return NULL;
}

std::vector<std::string> split(std::string s, char delim)
{
    std::vector<std::string> ret;
    std::istringstream iis;
    iis.clear();
    iis.str(s);
    std::string temp;
    while (std::getline(iis, temp, delim))
        ret.push_back(temp);
    return ret;
}

DWORD scanIdaStyle(std::string scanbytes, unsigned char* data, unsigned int length)
{
    std::vector<std::string> bytes = split(scanbytes, ' ');
    std::string mask = "", aob = "";
    for (auto& it : bytes)
    {
        if (it.size() && it[0] == '?')
        {
            mask += '?';
            aob += '\0';
        }
        else
        {
            mask += 'x';
            aob += (char)std::strtol(it.c_str(), NULL, 16);
        }
    }

    return dwFindPattern(data, length, reinterpret_cast<const unsigned char*>(aob.c_str()), mask.c_str());
}

void patchBuffer(DWORD start, std::string patch) {
    std::vector<std::string> bytes = split(patch, ' ');
    std::string norm_patch = "";

    for (std::string stuff : bytes) {
        norm_patch += static_cast<char>(std::strtol(stuff.c_str(), NULL, 16));
    }

    for (int i = 0; i < norm_patch.length(); i++) {
        *reinterpret_cast<char*>(start + i) = norm_patch[i];
    }

}

int main(int argc, char* argv[])
{

    if (argc == 2) {
        std::ifstream infile(argv[1], std::ios::binary);

        if (infile.good()) {
            // copies all data into buffer
            std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(infile), {});
            infile.close();

            DWORD ins = scanIdaStyle("83 CF 04 ?? 89 ?? 64", reinterpret_cast<unsigned char*>(&buffer[0]), buffer.size());
            patchBuffer(ins, "83 CF 05");


            std::ofstream outfile;
            outfile.open("RobloxStudioBeta_internal.exe", std::ios::trunc | std::ios::binary);
            outfile.write(reinterpret_cast<char*>(&buffer[0]), buffer.size());
            outfile.close();

        }
        else {
            std::cout << "unlucky";

        }

    }
    else {
        std::cout << "cringe World!\n";
    }


}

