#include <iostream>
#include "ControlManager.h"


//BESI
int main(int argc, char **argv) {
    std::cout << "Starting scada4home..." << std::endl;
    
    //TODO: An Logtracer et.al. übergeben    
    ControlManager *cMan = new ControlManager();
    cMan->Start();
    
     while(true)
    {    
      usleep(1000000);
    }
    
    cMan->Stop();
    
    return 0;
}
