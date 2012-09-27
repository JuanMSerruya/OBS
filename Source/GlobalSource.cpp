/********************************************************************************
 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


#include "Main.h"


class GlobalSource : public ImageSource
{
    ImageSource *globalSource;

    XElement *data;
    ClassInfo *sourceClass;

public:
    void Init(XElement *data)
    {
        this->data = data;
        UpdateSettings();
    }

    //called elsewhere automatically by the app
    //void Preprocess() {globalSource->Preprocess();}
    //void Tick(float fSeconds) {globalSource->Tick(fSeconds);}

    void Render(const Vect2 &pos, const Vect2 &size) {globalSource->Render(pos, size);}
    Vect2 GetSize() const {return globalSource->GetSize();}

    void UpdateSettings()
    {
        String strName = data->GetString(TEXT("name"));
        globalSource = App->GetGlobalSource(strName);
    }
};


ImageSource* STDCALL CreateGlobalSource(XElement *data)
{
    GlobalSource *source = new GlobalSource;
    source->Init(data);

    /*XElement *sourceElement = data->GetParent();

    Vect2 size = source->GetSize();
    sourceElement->SetInt(TEXT("cx"), int(size.x+EPSILON));
    sourceElement->SetInt(TEXT("cy"), int(size.y+EPSILON));*/

    return source;
}

bool STDCALL OBS::ConfigGlobalSource(XElement *element, bool bCreating)
{
    XElement *data = element->GetElement(TEXT("data"));
    CTSTR lpGlobalSourceName = data->GetString(TEXT("name"));

    XElement *globalSources = App->scenesConfig.GetElement(TEXT("global sources"));
    if(!globalSources) //shouldn't happen
        return false;

    XElement *globalSourceElement = globalSources->GetElement(lpGlobalSourceName);
    if(!globalSourceElement) //shouldn't happen
        return false;

    CTSTR lpClass = globalSourceElement->GetString(TEXT("class"));

    ClassInfo *classInfo = App->GetImageSourceClass(lpClass);
    if(!classInfo) //shouldn't happen
        return false;

    if(classInfo->configProc)
    {
        if(!classInfo->configProc(globalSourceElement, bCreating))
            return false;

        if(App->bRunning)
        {
            for(UINT i=0; i<App->globalSources.Num(); i++)
            {
                GlobalSourceInfo &info = App->globalSources[i];
                if(info.strName.CompareI(lpGlobalSourceName) && info.source)
                    info.source->UpdateSettings();
            }
        }
    }

    return true;
}

ImageSource* OBS::AddGlobalSourceToScene(CTSTR lpName)
{
    XElement *globals = scenesConfig.GetElement(TEXT("global sources"));
    if(globals)
    {
        XElement *globalSourceElement = globals->GetElement(lpName);
        if(globalSourceElement)
        {
            CTSTR lpClass = globalSourceElement->GetString(TEXT("class"));
            if(lpClass)
            {
                ImageSource *newGlobalSource = CreateImageSource(lpClass, globalSourceElement->GetElement(TEXT("data")));
                if(newGlobalSource)
                {
                    App->EnterSceneMutex();

                    GlobalSourceInfo *info = globalSources.CreateNew();
                    info->strName = lpName;
                    info->element = globalSourceElement;
                    info->source = newGlobalSource;

                    info->source->BeginScene();

                    App->LeaveSceneMutex();

                    return newGlobalSource;
                }
            }
        }
    }

    AppWarning(TEXT("OBS::AddGlobalSourceToScene: Could not find global source '%s'"), lpName);
    return NULL;
}

void OBS::GetGlobalSourceNames(List<CTSTR> &globalSourceNames)
{
    globalSourceNames.Clear();

    XElement *globals = scenesConfig.GetElement(TEXT("global sources"));
    if(globals)
    {
        UINT numSources = globals->NumElements();
        for(UINT i=0; i<numSources; i++)
        {
            XElement *sourceElement = globals->GetElementByID(i);
            globalSourceNames << sourceElement->GetName();
        }
    }
}

XElement* OBS::GetGlobalSourceElement(CTSTR lpName)
{
    XElement *globals = scenesConfig.GetElement(TEXT("global sources"));
    if(globals)
        return globals->GetElement(lpName);

    return NULL;
}