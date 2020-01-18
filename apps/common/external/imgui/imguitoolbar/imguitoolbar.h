// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.


#ifndef IMGUITOOLBAR_H_
#define IMGUITOOLBAR_H_

#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API

// SOME EXAMPLE CODE
/*
Outside any ImGui::Window:

        {
            static ImGui::Toolbar toolbar("myFirstToolbar##foo");
            if (toolbar.getNumButtons()==0)  {
                char tmp[1024];ImVec2 uv0(0,0),uv1(0,0);
                for (int i=0;i<9;i++) {
                    strcpy(tmp,"toolbutton ");
                    sprintf(&tmp[strlen(tmp)],"%d",i+1);
                    uv0 = ImVec2((float)(i%3)/3.f,(float)(i/3)/3.f);
                    uv1 = ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);

                    toolbar.addButton(ImGui::Toolbutton(tmp,(void*)myImageTextureId2,uv0,uv1));
                }
                toolbar.addSeparator(16);
                toolbar.addButton(ImGui::Toolbutton("toolbutton 11",(void*)myImageTextureId2,uv0,uv1,ImVec2(32,32),true,false,ImVec4(0.8,0.8,1.0,1)));  // Note that separator "eats" one toolbutton index as if it was a real button
                toolbar.addButton(ImGui::Toolbutton("toolbutton 12",(void*)myImageTextureId2,uv0,uv1,ImVec2(48,24),true,false,ImVec4(1.0,0.8,0.8,1)));  // Note that separator "eats" one toolbutton index as if it was a real button

                toolbar.setProperties(false,false,true,ImVec2(0.5f,0.f));
            }
            const int pressed = toolbar.render();
            if (pressed>=0) fprintf(stderr,"Toolbar1: pressed:%d\n",pressed);
        }
        {
            static ImGui::Toolbar toolbar("myFirstToolbar2##foo");
            if (toolbar.getNumButtons()==0)  {
                char tmp[1024];ImVec2 uv0(0,0),uv1(0,0);
                for (int i=8;i>=0;i--) {
                    strcpy(tmp,"toolbutton ");
                    sprintf(&tmp[strlen(tmp)],"%d",8-i+1);
                    uv0=ImVec2((float)(i%3)/3.f,(float)(i/3)/3.f);
                    uv1=ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);

                    toolbar.addButton(ImGui::Toolbutton(tmp,(void*)myImageTextureId2,uv0,uv1,ImVec2(24,48)));
                }
                toolbar.addSeparator(16);
                toolbar.addButton(ImGui::Toolbutton("toolbutton 11",(void*)myImageTextureId2,uv0,uv1,ImVec2(24,32),true,false,ImVec4(0.8,0.8,1.0,1)));  // Note that separator "eats" one toolbutton index as if it was a real button
                toolbar.addButton(ImGui::Toolbutton("toolbutton 12",(void*)myImageTextureId2,uv0,uv1,ImVec2(24,32),true,false,ImVec4(1.0,0.8,0.8,1)));  // Note that separator "eats" one toolbutton index as if it was a real button

                toolbar.setProperties(true,true,false,ImVec2(0.0f,0.0f),ImVec2(0.25f,0.9f),ImVec4(0.85,0.85,1,1));

                //toolbar.setScaling(2.0f,1.1f);
            }
            const int pressed = toolbar.render();
            if (pressed>=0) fprintf(stderr,"Toolbar2: pressed:%d\n",pressed);
        }


Inside a ImGui::Window:

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
            {
                static ImGui::Toolbar toolbar;
                if (toolbar.getNumButtons()==0)  {
                    char tmp[1024];ImVec2 uv0(0,0),uv1(0,0);
                    for (int i=0;i<9;i++) {
                        strcpy(tmp,"toolbutton ");
                        sprintf(&tmp[strlen(tmp)],"%d",i+1);
                        uv0 = ImVec2((float)(i%3)/3.f,(float)(i/3)/3.f);
                        uv1 = ImVec2(uv0.x+1.f/3.f,uv0.y+1.f/3.f);

                        toolbar.addButton(ImGui::Toolbutton(tmp,(void*)myImageTextureId2,uv0,uv1,ImVec2(16,16)));
                    }
                    toolbar.addSeparator(16);
                    toolbar.addButton(ImGui::Toolbutton("toolbutton 11",(void*)myImageTextureId2,uv0,uv1,ImVec2(16,16),true,true,ImVec4(0.8,0.8,1.0,1)));  // Note that separator "eats" one toolbutton index as if it was a real button
                    toolbar.addButton(ImGui::Toolbutton("toolbutton 12",(void*)myImageTextureId2,uv0,uv1,ImVec2(16,16),true,false,ImVec4(1.0,0.8,0.8,1)));  // Note that separator "eats" one toolbutton index as if it was a real button

                    toolbar.setProperties(true,false,false,ImVec2(0.0f,0.f),ImVec2(0.25,1));
                }
                const int pressed = toolbar.render();
                if (pressed>=0) fprintf(stderr,"Toolbar1: pressed:%d\n",pressed);
            }
            // Here we can open a child window if we want to toolbar not to scroll
            ImGui::Text("Hello");
            ImGui::End();
        }

*/

namespace ImGui {
class Toolbar {
public:
    struct Button {
        char tooltip[1024];
        void* user_texture_id;
        ImVec2 uv0;
        ImVec2 uv1;
        ImVec2 size;
        ImVec4 tint_col;
        bool isToggleButton;
        mutable bool isDown;
        inline Button(const char* _tooltip,void* _user_texture_id,const ImVec2& _uv0,const ImVec2& _uv1,const ImVec2& _size=ImVec2(32,32),bool _isToggleButton=false,bool _isDown=false,const ImVec4& _tint_col=ImVec4(1,1,1,1))
            : user_texture_id(_user_texture_id),uv0(_uv0),uv1(_uv1),size(_size),tint_col(_tint_col),isToggleButton(_isToggleButton),isDown(_isDown) {
            tooltip[0]='\0';
            if (_tooltip) {
                IM_ASSERT(strlen(_tooltip)<1024);
                strcpy(tooltip,_tooltip);
            }
        }
        inline Button(const Button& o) {*this=o;}
        inline const Button& operator=(const Button& o) {
            strcpy(tooltip,o.tooltip);
            user_texture_id=o.user_texture_id;
            size=o.size;
            uv0=o.uv0;uv1=o.uv1;tint_col=o.tint_col;
            isToggleButton=o.isToggleButton;
            isDown=o.isDown;
            return *this;
        }
    };


    Toolbar(const char* _name="",bool _visible=true,bool _keepAButtonSelected=false,bool _vertical=false,bool _lightAllBarWhenHovered=true,const ImVec2& _hvAlignmentsIn01=ImVec2(0.5f,0.f),const ImVec2& _opacityOffAndOn=ImVec2(-1.f,-1.f),const ImVec4& _bg_col=ImVec4(1,1,1,1),const ImVec4& _displayPortion=ImVec4(0,0,-1,-1))
    : visible(_visible),toolbarWindowPos(0,0),toolbarWindowSize(0,0),scaling(1,1),tooltipsDisabled(false)
    {
        IM_ASSERT(_name!=NULL && strlen(_name)<1024);  // _name must be valid
        strcpy(name,_name);
        setProperties(_keepAButtonSelected,_vertical,_lightAllBarWhenHovered,_hvAlignmentsIn01,_opacityOffAndOn,_bg_col,_displayPortion);
    }
    ~Toolbar() {clearButtons();}
    void setProperties(bool _keepAButtonSelected=false,bool _vertical=false,bool _lightAllBarWhenHovered=true,const ImVec2& _hvAlignmentsIn01=ImVec2(0.5f,0.f),const ImVec2& _opacityOffAndOn=ImVec2(-1.f,-1.f),const ImVec4& _bg_col=ImVec4(1,1,1,1),const ImVec4& _displayPortion=ImVec4(0,0,-1,-1)) {
        keepAButtonSelected = _keepAButtonSelected;
        vertical = _vertical;
        hvAlignmentsIn01 = _hvAlignmentsIn01;
        setDisplayProperties(_opacityOffAndOn,_bg_col);
        hoverButtonIndex = selectedButtonIndex = -1;
        lightAllBarWhenHovered=_lightAllBarWhenHovered;
        displayPortion = _displayPortion;

        if (buttons.size()>0) updatePositionAndSize();
    }
    void setDisplayProperties(const ImVec2& _opacityOffAndOn=ImVec2(-1.f,-1.f),const ImVec4& _bg_col=ImVec4(1,1,1,1)) {
        opacityOffAndOn = ImVec2(_opacityOffAndOn.x<0 ? 0.35f : _opacityOffAndOn.x,_opacityOffAndOn.y<0 ? 1.f : _opacityOffAndOn.y);
        bgColor = _bg_col;
    }

    int render(bool allowdHoveringWhileMouseDragging=false,int limitNumberOfToolButtonsToDisplay=-1) const {
        int pressedItem = -1;
        const int numberToolbuttons = (limitNumberOfToolButtonsToDisplay>=0 && limitNumberOfToolButtonsToDisplay<=buttons.size())?limitNumberOfToolButtonsToDisplay:(int)buttons.size();
        if (numberToolbuttons==0 || !visible) return pressedItem;
        const bool inWindowMode = name[0]=='\0';
        if (!inWindowMode && (toolbarWindowSize.x==0 || toolbarWindowSize.y==0)) updatePositionAndSize();

        if (inWindowMode) ImGui::PushID(this);
        // override sizes:
        ImGuiStyle& Style = ImGui::GetStyle();
        const ImVec2 oldWindowPadding = Style.WindowPadding;
        const ImVec2 oldItemSpacing = Style.ItemSpacing;
        if (!inWindowMode) Style.WindowPadding.x=Style.WindowPadding.y=0;
        Style.ItemSpacing.x=Style.ItemSpacing.y=0;              // Hack to fix vertical layout

        //
        if (!inWindowMode)  {
            ImGui::SetNextWindowPos(toolbarWindowPos);
            //if (keepAButtonSelected) ImGui::SetNextWindowFocus(); // This line was originally enabled
        }
        else if (keepAButtonSelected && selectedButtonIndex==-1) selectedButtonIndex=0;
        //
        static const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|
                ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings/*|ImGuiWindowFlags_Tooltip*/;
        const bool dontSkip = inWindowMode || ImGui::Begin(name,NULL,toolbarWindowSize,0,flags);
        if (dontSkip){                        
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));      // Hack not needed in previous ImGui versions: the bg color of the image button should be defined inside ImGui::ImageButton(...). (see below)
            bool noItemHovered = true;float nextSpacingAmount = 0;float currentWidth=Style.WindowPadding.x;const float windowWidth = ImGui::GetWindowWidth();
            float bg_col_opacity = 1.f;ImVec4 bg_col = bgColor;if (bg_col.w>=0) bg_col_opacity = bg_col.w;
            ImVec4 tint_col(1,1,1,1);ImVec2 tbsz(0,0);        
            const bool isMouseDragging = allowdHoveringWhileMouseDragging ? ImGui::IsMouseDragging(0) : false;
            for (int i=0;i<numberToolbuttons;i++)  {
                const Button& tb = buttons[i];
                tbsz.x=tb.size.x*scaling.x;tbsz.y=tb.size.y*scaling.y;
                if (inWindowMode)   {
                    if ((keepAButtonSelected && selectedButtonIndex==i) || (tb.isToggleButton && tb.isDown)) {bg_col.w = 0;tint_col.w = opacityOffAndOn.x;}
                    else {
                        bg_col.w = (i == hoverButtonIndex) ? bg_col_opacity : 0;tint_col = tb.tint_col;
                        tint_col.w = lightAllBarWhenHovered ?  ((hoverButtonIndex>=0) ? opacityOffAndOn.x : opacityOffAndOn.y) : ((i == hoverButtonIndex) ? opacityOffAndOn.x : opacityOffAndOn.y);
                    }
                }
                else {
                    if ((keepAButtonSelected && selectedButtonIndex==i) || (tb.isToggleButton && tb.isDown)) {bg_col.w = bg_col_opacity;tint_col.w = opacityOffAndOn.y;}
                    else {
                        bg_col.w = (i == hoverButtonIndex) ? bg_col_opacity : 0;tint_col = tb.tint_col;
                        tint_col.w = lightAllBarWhenHovered ?  ((hoverButtonIndex>=0) ? opacityOffAndOn.y : opacityOffAndOn.x) : ((i == hoverButtonIndex) ? opacityOffAndOn.y : opacityOffAndOn.x);
                    }
                }
                if (!vertical)  {
                    if (!tb.user_texture_id) {nextSpacingAmount+=tbsz.x;continue;}
                    else if (i>0) {
                        if (inWindowMode) {
                            currentWidth+=nextSpacingAmount;
                            if (tbsz.x<=windowWidth && currentWidth+tbsz.x>windowWidth) {
                                currentWidth=Style.WindowPadding.x;
                            }
                            else ImGui::SameLine(0,nextSpacingAmount);
                        }
                        else ImGui::SameLine(0,nextSpacingAmount);
                    }
                    nextSpacingAmount = 0;
                }
                else {
                    if (!tb.user_texture_id) {nextSpacingAmount = 0;continue;}
                    else if (i>0) {
                        if (i+1<numberToolbuttons)  {
                            const Button& tbNext = buttons[i+1];
                            if (!tbNext.user_texture_id) nextSpacingAmount+=tbNext.size.y*scaling.y;
                        }
                        ImGui::GetStyle().ItemSpacing.y=nextSpacingAmount;
                    }
                    nextSpacingAmount = 0;
                }
                ImGui::PushID(i);
                if (ImGui::ImageButton(tb.user_texture_id,tbsz,tb.uv0,tb.uv1,0,bg_col,tint_col)) {
                    if (tb.isToggleButton) tb.isDown=!tb.isDown;
                    else if (keepAButtonSelected) selectedButtonIndex = (selectedButtonIndex==i && !inWindowMode) ? -1 : i;
                    pressedItem=i;
                }
                ImGui::PopID();
                if (inWindowMode) currentWidth+=tbsz.x;
                const bool isItemHovered = isMouseDragging ? ImGui::IsItemHoveredRect() : ImGui::IsItemHovered();
                if (isItemHovered)
                {
                    if (!tooltipsDisabled && strlen(tb.tooltip)>0)   {
                        //if (!inWindowMode) ImGui::SetNextWindowFocus();
                        ImGui::SetTooltip("%s",tb.tooltip);

                        /*SetNextWindowPos(ImGui::GetIO().MousePos);
                        ImGui::BeginTooltip();
                        ImGui::Text(tb.tooltip);
                        ImGui::EndTooltip();*/
                    }
                    hoverButtonIndex = i;noItemHovered = false;
                }
            }
            if (noItemHovered) hoverButtonIndex = -1;
            ImGui::PopStyleColor();                             // Hack not needed in previous ImGui versions (see above)
        }
        if (!inWindowMode) ImGui::End();
        // restore old sizes
        Style.ItemSpacing = oldItemSpacing;     // Hack to fix vertical layout
        if (!inWindowMode) Style.WindowPadding = oldWindowPadding;
        if (inWindowMode) ImGui::PopID(); // this

        return pressedItem;
    }

    // called by other methods, but can be called manually after adding new buttons or resizing the screen.
    void updatePositionAndSize() const {
        const int numButtons = (int) buttons.size();
        toolbarWindowSize.x=toolbarWindowSize.y=0;
        if (numButtons==0 || name[0]=='\0') return;
        bool isSeparator;ImVec2 sz(0,0);
        for (int i=0;i<numButtons;i++) {
            const Button& b= buttons[i];
            isSeparator = b.user_texture_id == NULL;
            sz.x=b.size.x*scaling.x;sz.y=b.size.y*scaling.y;
            if (vertical)   {
                if (!isSeparator && toolbarWindowSize.x < sz.x) toolbarWindowSize.x = sz.x;
                toolbarWindowSize.y+=sz.y;
            }
            else {
                if (!isSeparator && toolbarWindowSize.y < sz.y) toolbarWindowSize.y = sz.y;
                toolbarWindowSize.x+=sz.x;
            }
        }    
        toolbarWindowPos.x = (displayPortion.x<0 ? (-ImGui::GetIO().DisplaySize.x*displayPortion.x) : displayPortion.x)+((displayPortion.z<0 ? (-ImGui::GetIO().DisplaySize.x*displayPortion.z) : displayPortion.z)-toolbarWindowSize.x)*hvAlignmentsIn01.x;
        toolbarWindowPos.y = (displayPortion.y<0 ? (-ImGui::GetIO().DisplaySize.y*displayPortion.y) : displayPortion.y)+((displayPortion.w<0 ? (-ImGui::GetIO().DisplaySize.y*displayPortion.w) : displayPortion.w)-toolbarWindowSize.y)*hvAlignmentsIn01.y;
    }

    inline size_t getNumButtons() const {return buttons.size();}
    inline Button* getButton(size_t i) {return ((int)i < buttons.size())? &buttons[i] : NULL;}
    inline const Button* getButton(size_t i) const {return ((int)i < buttons.size())? &buttons[i] : NULL;}
    inline void addButton(const Button& button) {
        buttons.push_back(button);
    }
    inline void addSeparator(float pixels) {
        buttons.push_back(Button("",NULL,ImVec2(0,0),ImVec2(0,0),ImVec2(pixels,pixels)));
    }
    inline void clearButtons() {
        for (int i=0;i<buttons.size();i++) buttons[i].~Button();
        buttons.clear();
    }

    inline void setVisible(bool flag) {visible = flag;}
    inline bool getVisible() {return visible;}

    inline void setSelectedButtonIndex(int index) const {selectedButtonIndex=index;}
    inline int getSelectedButtonIndex() const {return selectedButtonIndex;}
    inline bool isToggleButtonDown(int index) const {return (index>=0 && index<(int)buttons.size() && buttons[index].isToggleButton) ? buttons[index].isDown : false;}

    inline int getHoverButtonIndex() const {return hoverButtonIndex;}
    inline void setScaling(float x,float y) {
        if (scaling.x!=x || scaling.y!=y)   {
            scaling.x=x;scaling.y=y;
            updatePositionAndSize();
        }
    }
    const ImVec2& getScaling() const {return scaling;}
    inline void setDisplayPortion(const ImVec4& _displayPortion) {
        this->displayPortion = _displayPortion;
        updatePositionAndSize();
    }
    const ImVec4& getDisplayPortion() const {return this->displayPortion;}

    inline void disableTooltips(bool flag) {tooltipsDisabled=flag;}


    protected:
    char name[1024];
    bool visible;

#   ifdef _MSC_VER
    public:
#   endif //_MSC_VER

    bool keepAButtonSelected;

    mutable int hoverButtonIndex;
    mutable int selectedButtonIndex;

    bool vertical;
    ImVec2 hvAlignmentsIn01;
    ImVec2 opacityOffAndOn;
    ImVec4 bgColor;
    bool lightAllBarWhenHovered;
    mutable ImVec4 displayPortion;

    ImVector<Button> buttons;
    mutable ImVec2 toolbarWindowPos;
    mutable ImVec2 toolbarWindowSize;
    ImVec2 scaling;
    bool tooltipsDisabled;

    friend struct PanelManager;

};
typedef Toolbar::Button Toolbutton;

} // namespace Imgui


#endif //IMGUITOOLBAR_H_

