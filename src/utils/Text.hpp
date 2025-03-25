#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <glm/glm.hpp>

namespace text
{
    struct FontTable {
        size_t offset;
        size_t length;
    };
    struct GlyphData {
        unsigned unicodeValue;
    };
    struct Font
    {
    };
    inline constexpr bool isLittleEndian() { 
        int num = 1;
        return *(char *)&num == 1;
    }
    inline constexpr uint16_t readBigEndian(uint16_t const &value) {
        if(isLittleEndian()) {
            return value >> 8 | value << 8;
        } 
        return value;
    }
    inline constexpr int16_t readBigEndian(int16_t const &value) {
        if(isLittleEndian()) {
            return value >> 8 | value << 8;
        } 
        return value;
    }
    inline constexpr uint32_t readBigEndian(uint32_t const &value) {
        if(isLittleEndian()) {
            uint32_t a = (value >> 24) & 0b11111111;
            uint32_t b = (value >> 16) & 0b11111111;
            uint32_t c = (value >> 8)  & 0b11111111;
            uint32_t d = (value >> 0)  & 0b11111111;
            return a | b << 8 | c << 16 | d << 24;
        } 
        return value;
    }
    template<typename T> inline T readFromChar(char *data) {
        return *reinterpret_cast<T *>(data);
    }
    inline constexpr bool getBit(char byte, unsigned char index) { 
        return ((byte >> index) & 1) == 1;
    }
    inline Font loadFont(std::filesystem::path const &filePath) {
        Font font;

        std::ifstream file(filePath, std::ios::binary);
        std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
        size_t currentByte = 0;

        std::map<std::string, FontTable> tables;
        {
            currentByte += 4;
            unsigned numTables = readBigEndian(readFromChar<uint16_t>(&buffer[currentByte]));
            currentByte += 8;

            for(unsigned i = 0; i < numTables; ++i) {
                char tag[4];
                tag[0] = buffer[currentByte+0];
                tag[1] = buffer[currentByte+1];
                tag[2] = buffer[currentByte+2];
                tag[3] = buffer[currentByte+3];
                currentByte += 8;
                size_t offset = readBigEndian(readFromChar<uint32_t>(&buffer[currentByte]));
                currentByte += 4;
                size_t length = readBigEndian(readFromChar<uint32_t>(&buffer[currentByte]));
                currentByte += 4;
                tables[std::string(tag)] = { offset, length };
            }
        }
        
        {
            assert(tables.find("glyf") != tables.end());
            currentByte = tables.at("glyf").offset;

            int numberOfContours = readBigEndian(readFromChar<int16_t>(&buffer[currentByte]));
            if(numberOfContours > 0) { // simple glyph
                
            } else { // compound glyph
                numberOfContours = -numberOfContours;
            }
        }

        return font;
    }
} // namespace text
