﻿#ifndef MATERIALDATACOMPONENTPROPERTYEDITOR_H
#define MATERIALDATACOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "icomponentpropertyeditor.h"
#include "combolabeltext.h"
#include "../../Engine/Component/MaterialDataComponent.h"

class MaterialDataComponentPropertyEditor : public IComponentPropertyEditor
{
    Q_OBJECT
public:
    MaterialDataComponentPropertyEditor();

    void initialize() override;
    void edit(void* component) override;

    void GetMaterialAttributes();

private:
    QLabel* m_albedoLabel;
    ComboLabelText* m_albedoR;
    ComboLabelText* m_albedoG;
    ComboLabelText* m_albedoB;
    ComboLabelText* m_alpha;

    QLabel* m_MRATLabel;
    ComboLabelText* m_metallic;
    ComboLabelText* m_roughness;
    ComboLabelText* m_AO;
    ComboLabelText* m_thickness;

    ComboLabelText* m_shaderModel;

    MaterialDataComponent* m_component;

public slots:
    void SetMaterialAttributes();

    void remove() override;
};

#endif // MATERIALDATACOMPONENTPROPERTYEDITOR_H
