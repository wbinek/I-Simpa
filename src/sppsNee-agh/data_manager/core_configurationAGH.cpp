#include "Core_ConfigurationAGH.h"
#include <iostream>


Core_ConfigurationAGH::Core_ConfigurationAGH( CoreString xmlFilePath )
	:Core_Configuration(xmlFilePath)
{
	CXml fichierXml(xmlFilePath);

	CXmlNode* root=fichierXml.GetRoot();
	if(root)
	{
		CXmlNode* simuNode=root->GetChild("simulation");
		if(simuNode)
		{
			SetConfigInformation(I_PROP_CALCULATION_CORE_SELLECTION, simuNode->GetProperty("calculation_core").ToInt());

			CXmlNode* advancedNode = simuNode->GetChild("advanced");
			if (advancedNode)
			{
				uentier_long seed = 0;
				if (advancedNode->IsPropertyExist("random_seed")) {
					seed = advancedNode->GetProperty("random_seed").ToInt();
				}
				if (seed != 0) {
					// User define a random seed, multi-thread have to be deactivated in order to do the same computation
					SetConfigInformation(IPROP_DO_MULTITHREAD, 0);
					std::cout << "Random seed has been set; then multi-thread has been desactivated." << std::endl;
				}
				SetConfigInformation(I_PROP_RANDOM_SEED, seed);
			}
		}
	}
}


Core_ConfigurationAGH::~Core_ConfigurationAGH( )
{
}

//void Core_ConfigurationAGH::SetConfigInformation(FPROP propertyIndex,decimal valeur)
//{
//	Base_Core_Configuration::SetConfigInformation((BASE_FPROP)propertyIndex,valeur);
//}
//void Core_ConfigurationAGH::SetConfigInformation(SPROP propertyIndex,CoreString valeur)
//{
//	Base_Core_Configuration::SetConfigInformation((BASE_SPROP)propertyIndex,valeur);
//}
//void Core_ConfigurationAGH::SetConfigInformation(IPROP propertyIndex,entier valeur)
//{
//	Base_Core_Configuration::SetConfigInformation((BASE_IPROP)propertyIndex,valeur);
//}
//
//
//void Core_ConfigurationAGH::SetConfigInformation(BASE_FPROP propertyIndex,decimal valeur)
//{
//	Base_Core_Configuration::SetConfigInformation(propertyIndex,valeur);
//}
//void Core_ConfigurationAGH::SetConfigInformation(BASE_SPROP propertyIndex,CoreString valeur)
//{
//	Base_Core_Configuration::SetConfigInformation(propertyIndex,valeur);
//}
//void Core_ConfigurationAGH::SetConfigInformation(BASE_IPROP propertyIndex,entier valeur)
//{
//	Base_Core_Configuration::SetConfigInformation(propertyIndex,valeur);
//}
