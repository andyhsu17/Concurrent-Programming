#include <iostream>
#include <fstream>

int main()
{
    std::ofstream myfile("sourcefile.txt");
    for(int i = 1; i <= 100; i++)
    {
        myfile << i << std::endl;
    }
    myfile.close();
    return 0;
}