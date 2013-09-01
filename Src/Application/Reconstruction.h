#pragma once
#include "../Common/AppBase.h"
#include "../Tool/ViewTool.h"
#include "../DGP/PointCloud3D.h"
#include "ReconstructionUI.h"

namespace MagicApp
{
    class Reconstruction : public MagicCore::AppBase
    {
    public: 
        Reconstruction();
        virtual ~Reconstruction();

        virtual bool Enter(void);
        virtual bool Update(float timeElapsed);
        virtual bool Exit(void);
        virtual bool MouseMoved( const OIS::MouseEvent &arg );
        virtual bool MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

    private:
        void SetupScene(void);
        void ShutdownScene(void);
        
    private:
        ReconstructionUI mUI;
        MagicTool::ViewTool mViewTool;
    };
}