#include "ReconstructionApp.h"
#include "../Common/LogSystem.h"
#include "../Common/RenderSystem.h"
#include "../Common/ToolKit.h"

namespace MagicApp
{
    ReconstructionApp::ReconstructionApp() :
        mIsScannerDisplaying(false),
        mpPointSet(NULL),
        mpMesh(NULL), 
        mLeftLimit(-5000.f), 
        mRightLimit(5000.f), 
        mTopLimit(5000.f), 
        mDownLimit(-5000.f), 
        mFrontLimit(-500.f), 
        mBackLimit(-5000.f),
        mFrameStartIndex(0),
        mFrameEndIndex(0),
        mFrameCurrent(0),
        mIsSetFrameStart(false),
        mIsSetFrameEnd(false),
        mIsNeedRangeLimitCaculation(true)
    {
    }

    ReconstructionApp::~ReconstructionApp()
    {
    }

    bool ReconstructionApp::Enter(void)
    {
        MagicLog << "Enter ReconstructionApp" << std::endl;
        mUI.Setup();
        SetupRenderScene();
        return SetupDevice();
    }

    bool ReconstructionApp::Update(float timeElapsed)
    {
        if (mIsScannerDisplaying)
        {
            UpdateScannerDisplay();
        }
        return true;
    }

    bool ReconstructionApp::Exit(void)
    {
        ReleaseRenderScene();
        mUI.Shutdown();

        return true;
    }

    void ReconstructionApp::SetupRenderScene(void)
    {
        MagicCore::RenderSystem::GetSingleton()->GetMainCamera()->setPosition(0, 0, 500);
        Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::GetSingleton()->GetSceneManager();
        pSceneMgr->setAmbientLight(Ogre::ColourValue(0.1, 0.1, 0.1));
        Ogre::Light* frontLight = pSceneMgr->createLight("frontLight");
        frontLight->setPosition(0, 0, 500);
        frontLight->setDiffuseColour(0.8, 0.8, 0.8);
        frontLight->setSpecularColour(0.5, 0.5, 0.5);
    }

    void ReconstructionApp::ReleaseRenderScene(void)
    {
        Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::GetSingleton()->GetSceneManager();
        pSceneMgr->destroyLight("frontLight");
        MagicCore::RenderSystem::GetSingleton()->SetupCameraDefaultParameter();
    }

    bool ReconstructionApp::SetupDevice()
    {
        if (MagicCore::ToolKit::GetSingleton()->IsONIInitialized() == false)
        {
            openni::Status rc = openni::OpenNI::initialize();
            if (rc != openni::STATUS_OK)
            {
                MagicLog << "OpenNI initialize failed: " << openni::OpenNI::getExtendedError() << std::endl;
                return false;
            }
            else
            {
                MagicLog << "OpenNI initialize succeed" << std::endl;
                MagicCore::ToolKit::GetSingleton()->SetONIInitialized(true);
            }
        }

        return true;
    }

    void ReconstructionApp::ReleaseDevice()
    {

    }

    void ReconstructionApp::CoarseRangeLimitCalculation(const std::vector<MagicDGP::Vector3>& posList)
    {
        mLeftLimit = mDownLimit = 5000.f;
        mRightLimit = mTopLimit = -5000.f;
        mFrontLimit = -5000.f;
        mBackLimit = -500.f;
        int posNum = posList.size();
        for (int i = 0; i < posNum; i++)
        {
            MagicDGP::Vector3 pos = posList.at(i);
            if (pos[0] < mLeftLimit)
            {
                mLeftLimit = pos[0];
            }
            if (pos[0] > mRightLimit)
            {
                mRightLimit = pos[0];
            }
            if (pos[1] < mDownLimit)
            {
                mDownLimit = pos[1];
            }
            if (pos[1] > mTopLimit)
            {
                mTopLimit = pos[1];
            }
            if (pos[2] > -(MagicDGP::Epsilon))
            {
                continue;
            }
            if (pos[2] > mFrontLimit)
            {
                mFrontLimit = pos[2];
            }
            if (pos[2] < mBackLimit)
            {
                mBackLimit = pos[2];
            }
        }
        MagicLog << "Coarse Limit: " << mLeftLimit << " " << mRightLimit << " " << mDownLimit << " " << mTopLimit << 
            " " << mFrontLimit << " " << mBackLimit << std::endl;
    }

    void ReconstructionApp::UpdateScannerDisplay()
    {
        if (mIsSetFrameStart)
        {
            mFrameStartIndex = mFrameCurrent;
            mIsSetFrameStart = false;
        }
        if (mIsSetFrameEnd)
        {
            mFrameEndIndex = mFrameCurrent;
            mIsSetFrameEnd = false;
        }
        openni::PlaybackControl* pPC = mDevice.getPlaybackControl();
        pPC->seek(mDepthStream, mFrameCurrent);
        mFrameCurrent++;
        if (mFrameCurrent > mFrameEndIndex)
        {
            mFrameCurrent = mFrameStartIndex;
        }

        openni::VideoFrameRef depthFrame;
        int changeIndex;
        openni::VideoStream** pStreams = new openni::VideoStream*[1];
        pStreams[0] = &mDepthStream;
        openni::OpenNI::waitForAnyStream(pStreams, 1, &changeIndex);
        if (changeIndex == 0)
        {
            mDepthStream.readFrame(&depthFrame);
        }
        delete []pStreams;

        if (depthFrame.isValid())
        {
            const openni::DepthPixel* pDepth = (const openni::DepthPixel*)depthFrame.getData();
            int resolutionX = depthFrame.getVideoMode().getResolutionX();
            int resolutionY = depthFrame.getVideoMode().getResolutionY();
            std::vector<MagicDGP::Vector3> posList;
            for(int y = 0; y < resolutionY; y++)  
            {  
                for(int x = 0; x < resolutionX; x++)  
                {
                    openni::DepthPixel depth = pDepth[y * resolutionX + x]; 
                    float rx, ry, rz;
                    openni::CoordinateConverter::convertDepthToWorld(mDepthStream, 
                        x, y, depth, &rx, &ry, &rz);
                    MagicDGP::Vector3 pos(-rx, ry, -rz);
                    posList.push_back(pos);
                }
            }
            if (mIsNeedRangeLimitCaculation)
            {
                CoarseRangeLimitCalculation(posList);
                mIsNeedRangeLimitCaculation = false;
            }
            std::vector<MagicDGP::Vector3> norList;
            for (int y = 0; y < resolutionY; y++)
            {
                for (int x = 0; x < resolutionX; x++)
                {
                    if ((y == 0) || (y == resolutionY - 1) || (x == 0) || (x == resolutionX - 1))
                    {
                        norList.push_back(MagicDGP::Vector3(0, 0, 1));
                        continue;
                    }
                    MagicDGP::Vector3 pos = posList.at(y * resolutionX + x);
                    if (pos[0] < mLeftLimit || pos[0] > mRightLimit ||
                        pos[1] < mDownLimit || pos[1] > mTopLimit ||
                        pos[2] > mFrontLimit || pos[2] < mBackLimit)
                    {
                        norList.push_back(MagicDGP::Vector3(0, 0, 1));
                        continue;
                    }
                    MagicDGP::Vector3 dirX = posList.at(y * resolutionX + x + 1) - posList.at(y * resolutionX + x - 1);
                    MagicDGP::Vector3 dirY = posList.at((y + 1) * resolutionX + x) - posList.at((y - 1) * resolutionX + x);
                    MagicDGP::Vector3 nor = dirX.CrossProduct(dirY);
                    MagicDGP::Real len = nor.Normalise();
                    if (len > MagicDGP::Epsilon)
                    {
                        norList.push_back(nor);
                    }
                    else
                    {
                        norList.push_back(MagicDGP::Vector3(0, 0, 1));
                    }
                }
            }
            //Rendering Point Set
            Ogre::ManualObject* pMObj = NULL;
            Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::GetSingleton()->GetSceneManager();
            char psName[20] = "ScannerDepth";
            if (pSceneMgr->hasManualObject(psName))
            {
                pMObj = pSceneMgr->getManualObject(psName);
                pMObj->clear();
            }
            else
            {
                pMObj = pSceneMgr->createManualObject(psName);
                pSceneMgr->getRootSceneNode()->attachObject(pMObj);
            }
            pMObj->begin("SimplePoint", Ogre::RenderOperation::OT_POINT_LIST);
            int pointNum = posList.size();
            for (int i = 0; i < pointNum; i++)
            {
                MagicDGP::Vector3 pos = posList.at(i);
                if (pos[0] < mLeftLimit || pos[0] > mRightLimit ||
                    pos[1] < mDownLimit || pos[1] > mTopLimit ||
                    pos[2] > mFrontLimit || pos[2] < mBackLimit)
                {
                    continue;
                }
                MagicDGP::Vector3 nor = norList.at(i);
                pMObj->position(pos[0], pos[1], pos[2]);
                pMObj->normal(nor[0], nor[1], nor[2]);
            }
            pMObj->end();
        }
    }

    bool ReconstructionApp::OpenSceneRecord()
    {
        if (mIsScannerDisplaying)
        {
            mDevice.close();
            mDepthStream.destroy();
        }
        std::string fileName;
        MagicCore::ToolKit::GetSingleton()->FileOpenDlg(fileName);
        openni::Status rc = mDevice.open(fileName.c_str());
        if (rc != openni::STATUS_OK)
        {
            MagicLog << "ReconstructionApp::OpenSceneRecord failed: " << openni::OpenNI::getExtendedError() << std::endl;
            return false;
        }
        rc = mDepthStream.create(mDevice, openni::SENSOR_DEPTH);
        if (rc != openni::STATUS_OK)
        {
            MagicLog << "DepthStream create failed: " << openni::OpenNI::getExtendedError() << std::endl;
            return false;
        }
        mDepthStream.start();
        mIsScannerDisplaying = true;
        mIsNeedRangeLimitCaculation = true;
        openni::PlaybackControl* pPC = mDevice.getPlaybackControl();
        mFrameStartIndex = 0;
        mFrameEndIndex = pPC->getNumberOfFrames(mDepthStream);
        mFrameCurrent = 0;

        return true;
    }

    void ReconstructionApp::SetTimeStart()
    {
        mIsSetFrameStart = true;
    }

    void ReconstructionApp::SetTimeEnd()
    {
        mIsSetFrameEnd = true;
        MagicLog << "Coarse Limit: " << mLeftLimit << " " << mRightLimit << " " << mDownLimit << " " << mTopLimit << 
             " " << mFrontLimit << " " << mBackLimit << std::endl;
    }

    void ReconstructionApp::ChangeLeftRange(int rel)
    {
        float temp = mLeftLimit + rel;
        if (temp > -5000.f && temp < mRightLimit)
        {
            mLeftLimit = temp;
        }
    }

    void ReconstructionApp::ChangeRightRange(int rel)
    {
        float temp = mRightLimit + rel;
        if (temp < 5000.f && temp > mLeftLimit)
        {
            mRightLimit = temp;
        }
    }

    void ReconstructionApp::ChangeTopRange(int rel)
    {
        float temp = mTopLimit + rel;
        if (temp < 5000.f && temp > mDownLimit)
        {
            mTopLimit = temp;
        }
    }

    void ReconstructionApp::ChangeDownRange(int rel)
    {
        float temp = mDownLimit + rel;
        if (temp > -5000.f && temp < mTopLimit)
        {
            mDownLimit = temp;
        }
    }

    void ReconstructionApp::ChangeFrontRange(int rel)
    {
        float temp = mFrontLimit - rel;
        if (temp < -500.f && temp > mBackLimit)
        {
            mFrontLimit = temp;
        }
    }

    void ReconstructionApp::ChangeBackRange(int rel)
    {
        float temp = mBackLimit - rel;
        if (temp > -5000.f && temp < mFrontLimit)
        {
            mBackLimit = temp;
        }
    }

    void ReconstructionApp::StartRegistration()
    {
        mIsScannerDisplaying = false;
        //
        // Do Registration
        //
        mUI.StartPostProcess();
    }

    bool ReconstructionApp::SavePointSet()
    {
        return true;
    }

    bool ReconstructionApp::ReconstructPointSet()
    {
        return true;
    }

    bool ReconstructionApp::SaveMesh3D()
    {
        return true;
    }

    void ReconstructionApp::SmoothMesh3D()
    {

    }
}