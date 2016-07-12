#include "NextEventEstimationCore.h"
#include "tools/collision.h"
#include "tools/dotreflection.h"
#include "tools/brdfreflection.h"
#include "tools/dotdistribution.h"
#include <iostream>

NextEventEstimationCore::NextEventEstimationCore(t_Mesh& _sceneMesh, t_TetraMesh& _sceneTetraMesh, CONF_CALCULATION &_confEnv, Core_Configuration &_configurationTool, ReportManager* _reportTool)
	: CalculationCore(_sceneMesh, _sceneTetraMesh, _confEnv, _configurationTool, _reportTool) 
{
	doDirectSoundCalculation = true;
};


void NextEventEstimationCore::Movement(CONF_PARTICULE &configurationP)
{
	decimal deltaT = *configurationTool->FastGetConfigValue(Core_Configuration::FPROP_TIME_STEP);
	decimal distanceSurLePas = configurationP.direction.length();
	decimal celeriteLocal = distanceSurLePas / deltaT;
	decimal faceDirection;
	bool collisionResolution = true; //On test de nouveau la collision dans le pas de temps courant si cette valeur est � vrai
	int iteration = 0;
	decimal distanceCollision = 0.f;
	decimal distanceToTravel = 0.f;
	while (collisionResolution && configurationP.stateParticule == PARTICULE_STATE_ALIVE)
	{
		iteration++;
		collisionResolution = false;
		//Si il y a collision avec une face (avec prise en compte de la distance parcourue)
		distanceCollision = (configurationP.nextModelIntersection.collisionPosition - configurationP.position).length();
		distanceToTravel = celeriteLocal*(deltaT - configurationP.elapsedTime);

		//Test de collision avec un �l�ment de l'encombrement entre la position de la particule et une face du tetrah�dre courant.
		if (*configurationTool->FastGetConfigValue(Core_Configuration::IPROP_DO_CALC_ENCOMBREMENT) && distanceToTravel >= configurationP.distanceToNextEncombrementEle && distanceCollision>configurationP.distanceToNextEncombrementEle && configurationP.currentTetra->volumeEncombrement)
		{
			//Collision avec un �l�ment virtuel de l'encombrement courant

			//Test d'absorption

			if (*configurationTool->FastGetConfigValue(Core_Configuration::IPROP_ENERGY_CALCULATION_METHOD))
			{
				//Energ�tique
				configurationP.energie *= (1 - configurationP.currentTetra->volumeEncombrement->encSpectrumProperty[configurationP.frequenceIndex].alpha);
				if (configurationP.energie <= configurationP.energie_epsilon)
				{
					configurationP.stateParticule = PARTICULE_STATE_ABS_ENCOMBREMENT;
					return;
				}
			}
			else {
				//Al�atoire
				if (GetRandValue() <= configurationP.currentTetra->volumeEncombrement->encSpectrumProperty[configurationP.frequenceIndex].alpha)
				{
					//Absorb�
					configurationP.energie = 0.f;
					configurationP.stateParticule = PARTICULE_STATE_ABS_ENCOMBREMENT;
					return;
				}
			}
			//N'est pas absorb�

			//On incr�mente le temps de parcourt entre la position avant et apr�s collision avec l'encombrement virtuel
			configurationP.elapsedTime += configurationP.distanceToNextEncombrementEle / celeriteLocal;
			//On place la particule sur la position de collision
			FreeParticleTranslation(configurationP, (configurationP.direction / configurationP.direction.length())*configurationP.distanceToNextEncombrementEle);
			collisionResolution = true;
			//On change la direction de la particule en fonction de la loi de distribution
			vec3 newDir;
			switch (configurationP.currentTetra->volumeEncombrement->encSpectrumProperty[configurationP.frequenceIndex].law_diffusion)
			{
			case DIFFUSION_LAW_UNIFORM:
				ParticleDistribution::GenSphereDistribution(configurationP, configurationP.direction.length());
				break;
			case DIFFUSION_LAW_REFLEXION_UNIFORM:
				newDir = ReflectionLaws::FittingUniformReflection(configurationP.direction);
				newDir.normalize();
				configurationP.direction = newDir*configurationP.direction.length();
				break;
			case DIFFUSION_LAW_REFLEXION_LAMBERT:
				newDir = ReflectionLaws::FittingLambertReflection(configurationP.direction);
				newDir.normalize();
				configurationP.direction = newDir*configurationP.direction.length();
				break;

			};
			//Calcul du nouveau point de collision
			SetNextParticleCollision(configurationP);
			SetNextParticleCollisionWithObstructionElement(configurationP);
		}
		else if (distanceCollision <= distanceToTravel) // && configurationP.nextModelIntersection.idface!=-1
		{
			//Enregistrement de l'�nergie pass� � la paroi
			reportTool->ParticuleCollideWithSceneMesh(configurationP);

			vec3 vecTranslation = configurationP.nextModelIntersection.collisionPosition - configurationP.position;
			//On incr�mente le temps de parcourt entre la position avant et apr�s collision
			configurationP.elapsedTime += (vecTranslation / configurationP.direction.length()).length()*deltaT;

			//On place la particule sur la position de collision
			FreeParticleTranslation(configurationP, vecTranslation);

			// R�cuperation de l'information de la face
			t_cFace* faceInfo = NULL;

#ifdef UTILISER_MAILLAGE_OPTIMISATION
			faceInfo = configurationP.currentTetra->faces[configurationP.nextModelIntersection.idface].face_scene;

			//test de passage d'un t�tra�dre � un autre

			//Vrai si la paroi est anormalement orient�e
			bool doInvertNormal(false);
			if (faceInfo)
			{
				faceDirection = configurationP.direction.dot(faceInfo->normal);
				doInvertNormal = (faceDirection <= -BARELY_EPSILON);
			}
			//On traverse la paroi du tetrahedre si (pas de r�solution de collision si)
			//	- Ce n'est pas une surface du mod�le
			//  - (ou) Elle n'est pas orient�e vers nous et le mat�riau n'affecte les surfaces sur une orientation
			//  - (ou) Cette surface est un encombrement et qu'un autre volume nous attend derri�re
			if (!faceInfo || ((faceInfo->faceEncombrement || (!(faceInfo->faceMaterial->doubleSidedMaterialEffect) && doInvertNormal)) && configurationP.currentTetra->voisins[configurationP.nextModelIntersection.idface]))
			{
				TraverserTetra(configurationP, collisionResolution);
			}
			else {
#else
			faceInfo = &sceneMesh->pfaces[configurationP.nextModelIntersection.idface];
			///////////////////////////////////
			// Test de passage d'un milieu libre � un milieu encombr� (et inversement)
			if (!faceInfo->faceEncombrement)
			{
#endif

				//On stocke le materiau dans la variable materialInfo
				t_Material_BFreq* materialInfo = &(*faceInfo).faceMaterial->matSpectrumProperty[configurationP.frequenceIndex];


				bool transmission = false;
				//Tirage al�atoire pour le test d'absorption
				if (*configurationTool->FastGetConfigValue(Core_Configuration::IPROP_DO_CALC_CHAMP_DIRECT))
				{
					//Particule absorb�e
					if (configurationP.stateParticule == PARTICULE_STATE_ALIVE)
						configurationP.stateParticule = PARTICULE_STATE_ABS_SURF;
					configurationP.energie = 0.f;
					return;
				}
				else {
					if (*configurationTool->FastGetConfigValue(Core_Configuration::IPROP_ENERGY_CALCULATION_METHOD))
					{
						//Methode �n�rg�tique, particule en collision avec la paroi
						//Particule courante = (1-alpha)*epsilon
						//Si l'absorption est totale la particule est absorb�e si tau=0
						if (materialInfo->absorption == 1) //Pas de duplication possible de la particule (forcement non r�fl�chie)
						{
							if (!materialInfo->dotransmission || !(*configurationTool->FastGetConfigValue(Core_Configuration::IPROP_DO_CALC_TRANSMISSION)))
							{
								if (configurationP.stateParticule == PARTICULE_STATE_ALIVE)
									configurationP.stateParticule = PARTICULE_STATE_ABS_SURF;
								configurationP.energie = 0;
								return;
							}
							else {
								transmission = true;
								configurationP.energie *= materialInfo->tau;
							}
						}
						else {
							if (materialInfo->absorption != 0) //Pas de duplication possible de la particule (forcement r�fl�chie)
							{
								if (materialInfo->dotransmission && materialInfo->tau != 0 && configurationP.energie*materialInfo->tau>configurationP.energie_epsilon && (*configurationTool->FastGetConfigValue(Core_Configuration::IPROP_DO_CALC_TRANSMISSION)))
								{
									//On va dupliquer la particule
									CONF_PARTICULE configurationPTransmise = configurationP;
									configurationPTransmise.energie *= materialInfo->tau;
									bool localcolres;
									TraverserTetra(configurationPTransmise, localcolres);
									//configurationPTransmise.currentTetra=configurationP.currentTetra->voisins[configurationP.nextModelIntersection.idface];
									if (configurationPTransmise.energie>configurationPTransmise.energie_epsilon)
									{
										confEnv.duplicatedParticles.push_back(configurationPTransmise);
									}
								}
								configurationP.energie *= (1 - materialInfo->absorption);
							} //else reflexion sans absorption
						}
					}
					else {
						//Test d'absorption en al�atoire
						if (GetRandValue() <= materialInfo->absorption)
						{
							// Particule non r�fl�chie
							if ((*configurationTool->FastGetConfigValue(Core_Configuration::IPROP_DO_CALC_TRANSMISSION)) && materialInfo->dotransmission && configurationP.currentTetra->voisins[configurationP.nextModelIntersection.idface] && GetRandValue()*materialInfo->absorption <= materialInfo->tau)
							{
								transmission = true;
							}
							else {
								//Particule absorb�e
								if (configurationP.stateParticule == PARTICULE_STATE_ALIVE)
									configurationP.stateParticule = PARTICULE_STATE_ABS_SURF;
								configurationP.energie = 0.;
								return;
							}
						}
					}
				}
				if (configurationP.energie <= configurationP.energie_epsilon)
				{
					if (configurationP.stateParticule == PARTICULE_STATE_ALIVE)
						configurationP.stateParticule = PARTICULE_STATE_ABS_SURF;
					return;
				}
				//Si Transmission on traverse la paroi
				if (transmission)
				{
					TraverserTetra(configurationP, collisionResolution);
				}
				else {
					// Choix de la m�thode de reflexion en fonction de la valeur de diffusion
					vec3 nouvDirection;
					vec3 faceNormal;
					if (!doInvertNormal)
						faceNormal = -faceInfo->normal;
					else
						faceNormal = faceInfo->normal;

					//Get direction for diffuse or specular part based on material info
					if (materialInfo->diffusion == 1 || GetRandValue()<materialInfo->diffusion)
					{
						nouvDirection = ReflectionLaws::SolveDiffusePart(configurationP.direction, *materialInfo, faceNormal, configurationP);
					}
					else {
						nouvDirection = ReflectionLaws::SolveSpecularPart(configurationP.direction, *materialInfo, faceNormal, configurationP);
					}

					//Calculate and cast shadow rays
					for each (t_Recepteur_P* receiver in configurationTool->recepteur_p_List)
					{
						CONF_PARTICULE shadowRay = configurationP;
						vec3 newDirection, toReceiver;

						toReceiver = receiver->position - shadowRay.position;
						newDirection = toReceiver;
						newDirection.normalize();
						shadowRay.direction = newDirection*distanceSurLePas;

						if (VisabilityTest(shadowRay, receiver->position))
						{
							shadowRay.targetReceiver = receiver;
							shadowRay.isShadowRay = true;

							float energy = BRDFs::SolveBRDFReflection(*materialInfo, faceInfo->normal, shadowRay, configurationP.direction, configurationTool);
							shadowRay.energie *= energy;
							shadowRay.energie_epsilon = 0.05* shadowRay.energie;

							//fast forward particle to receiver surrounding
							int timeStepNum = (toReceiver.length() - ((deltaT - configurationP.elapsedTime) / deltaT) - *configurationTool->FastGetConfigValue(Core_Configuration::FPROP_RAYON_RECEPTEURP)) / distanceSurLePas;

							decimal densite_proba_absorption_atmospherique = configurationTool->freqList[configurationP.frequenceIndex]->densite_proba_absorption_atmospherique;
							shadowRay.position = shadowRay.position + shadowRay.direction * (timeStepNum + (deltaT - configurationP.elapsedTime) / deltaT);
							shadowRay.elapsedTime = 0;
							shadowRay.pasCourant += timeStepNum;
							shadowRay.currentTetra = &sceneTetraMesh->tetraedres[receiver->indexTetra];
							shadowRay.energie *= pow(densite_proba_absorption_atmospherique, timeStepNum);

							confEnv.duplicatedParticles.push_back(shadowRay);

						}
					}


					//Calcul de la nouvelle direction de r�flexion (en reprenant la c�l�rit� de propagation du son)
					configurationP.direction = nouvDirection*distanceSurLePas;
					collisionResolution = true;
					SetNextParticleCollision(configurationP);
				}
			}
			}

		if (iteration>1000)
		{
			//Elle est d�truite et l'utilisateur en sera inform�
			if (configurationP.stateParticule == PARTICULE_STATE_ALIVE)
				configurationP.stateParticule = PARTICULE_STATE_LOOP;
			configurationP.energie = 0;
			return;
		}
		}
	if (configurationP.elapsedTime == 0.f)
	{   //Aucune collision sur le pas de temps courant
		FreeParticleTranslation(configurationP, configurationP.direction);
	}
	else {
		//Il y a eu une ou plusieurs collisions sur le pas de temps courant
		FreeParticleTranslation(configurationP, configurationP.direction*((deltaT - configurationP.elapsedTime) / deltaT));
		configurationP.elapsedTime = 0; //remise du compteur � 0
	}
}


void NextEventEstimationCore::FreeParticleTranslation(CONF_PARTICULE &configurationP, const vec3 &translationVector)
{
	if(configurationP.isShadowRay) reportTool->ShadowRayFreeTranslation(configurationP, configurationP.position + translationVector);
	// On prend en compte le rapprochement vers l'encombrement virtuel
	if (configurationP.currentTetra->volumeEncombrement)
		configurationP.distanceToNextEncombrementEle -= translationVector.length();
	configurationP.position += translationVector;
}