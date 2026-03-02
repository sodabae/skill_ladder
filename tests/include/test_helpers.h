#pragma once

#include <iostream>
#include <sstream>
#include <string>

class CoutCapture
{
public:
    CoutCapture() : m_old(std::cout.rdbuf(m_stream.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(m_old); }

    std::string str() const { return m_stream.str(); }

private:
    std::ostringstream m_stream;
    std::streambuf* m_old;
};
