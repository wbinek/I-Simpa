//#include "coreTypes.h"
#include "tools/dotreflection.h"


/**
 * @brief Cette classe regroupe les méthodes de réfléxion de particules sur les surfaces
 *
 * Les vecteurs de réflexion retournés sont normalisés car une perte de précision se fait ressentir quand à la norme de ces vecteurs
 */
class ReflectionLawsAGH : public  ReflectionLaws
{
public:
	/**
	 * En fonction du matériau appel la méthode de réflexion associé. Si la méthode est inconnue la réflexion est spéculaire.
	 * @param vectorDirection Vecteur incident
	 * @param materialInfo Matériau de la face
	 * @param faceInfo Informations de la face en collision
	 * @param AB Vecteur perpendiculaire à la normal de la face
	 * @return Vecteur réflexion normalisé
	 */
	static vec3 SolveDiffusePart(vec3 &vectorDirection,t_Material_BFreq &materialInfo,vec3& faceNormal, CONF_PARTICULE_AGH& particuleInfo)
	{
		switch(materialInfo.reflectionLaw)
		{
			case REFLECTION_LAW_SPECULAR:
				return SpecularReflection(vectorDirection,faceNormal);
				break;
			case REFLECTION_LAW_LAMBERT:
				return BaseWnReflection(vectorDirection,faceNormal,1,particuleInfo);
				break;
			case REFLECTION_LAW_UNIFORM:
				return BaseWnReflection(vectorDirection,faceNormal,0,particuleInfo);
				break;
			case REFLECTION_LAW_W2:
				return BaseWnReflection(vectorDirection,faceNormal,2,particuleInfo);
				break;
			case REFLECTION_LAW_W3:
				return BaseWnReflection(vectorDirection,faceNormal,3,particuleInfo);
				break;
			case REFLECTION_LAW_W4:
				return BaseWnReflection(vectorDirection,faceNormal,4,particuleInfo);
				break;
			case REFLECTION_LAW_PHONG:
				return BaseWnReflection(vectorDirection, faceNormal, 1, particuleInfo);
				break;
			default:
				return SpecularReflection(vectorDirection,faceNormal);
		};
	}

	static vec3 SolveSpecularPart(vec3 &vectorDirection, t_Material_BFreq &materialInfo, vec3& faceNormal, CONF_PARTICULE_AGH& particuleInfo)
	{
		switch (materialInfo.reflectionLaw)
		{
		case REFLECTION_LAW_PHONG:
			return PhongSpecularPart(vectorDirection, faceNormal, particuleInfo, materialInfo);
			break;
		default:
			return SpecularReflection(vectorDirection, faceNormal);
		};
	}


private:

	static vec3 PhongSpecularPart(vec3 &vectorDirection, vec3 &faceNormal, CONF_PARTICULE_AGH& particuleInfo, t_Material_BFreq &material)
	{
		float n = powf(10, powf(-0.82662*material.diffusion, 3) + 1.5228);
		vec3 specularDir = SpecularReflection(vectorDirection, faceNormal);

		Matrix3 rotationMatrix;
		rotationMatrix.calculateRotationMatrix(faceNormal, specularDir);

		vec3 target = BaseWnReflection(vectorDirection, faceNormal, n, particuleInfo);
		target = rotationMatrix*target;

		double test = target.dot(faceNormal);

		//test if reflected ray is pointing into ground
		if(target.dot(faceNormal) >= M_PI/2)
		{
			//if it is get new random direction
			target = PhongSpecularPart(vectorDirection, faceNormal, particuleInfo, material);
		}

		return target/target.length();
	}
};


/**
* @brief Cette classe regroupe les méthodes de réfléxion de particules sur les surfaces
*
* Les vecteurs de réflexion retournés sont normalisés car une perte de précision se fait ressentir quand à la norme de ces vecteurs
*/
class ReflectionLawsMLT : public  ReflectionLaws
{
public:
	/**
	* En fonction du matériau appel la méthode de réflexion associé. Si la méthode est inconnue la réflexion est spéculaire.
	* @param vectorDirection Vecteur incident
	* @param materialInfo Matériau de la face
	* @param faceInfo Informations de la face en collision
	* @param AB Vecteur perpendiculaire à la normal de la face
	* @return Vecteur réflexion normalisé
	*/
	static vec3 SolveDiffusePart(vec3 &vectorDirection, t_Material_BFreq &materialInfo, vec3& faceNormal, CONF_PARTICULE_AGH& particuleInfo,double &rnd1, double &rnd2, double &probability)
	{
		switch (materialInfo.reflectionLaw)
		{
		case REFLECTION_LAW_SPECULAR:
			probability = (1 - materialInfo.diffusion);
			return SpecularReflection(vectorDirection, faceNormal);
			break;
		case REFLECTION_LAW_LAMBERT:
			return BaseWnReflection(vectorDirection, faceNormal, 1, particuleInfo, rnd1, rnd2, probability);
			break;
		case REFLECTION_LAW_UNIFORM:
			return BaseWnReflection(vectorDirection, faceNormal, 0, particuleInfo, rnd1, rnd2, probability);
			break;
		case REFLECTION_LAW_W2:
			return BaseWnReflection(vectorDirection, faceNormal, 2, particuleInfo, rnd1, rnd2, probability);
			break;
		case REFLECTION_LAW_W3:
			return BaseWnReflection(vectorDirection, faceNormal, 3, particuleInfo, rnd1, rnd2, probability);
			break;
		case REFLECTION_LAW_W4:
			return BaseWnReflection(vectorDirection, faceNormal, 4, particuleInfo, rnd1, rnd2, probability);
			break;
		case REFLECTION_LAW_PHONG:
			return BaseWnReflection(vectorDirection, faceNormal, 1, particuleInfo, rnd1, rnd2, probability);
			break;
		default:
			return SpecularReflection(vectorDirection, faceNormal);
		};
	}

	static vec3 SolveSpecularPart(vec3 &vectorDirection, t_Material_BFreq &materialInfo, vec3& faceNormal, CONF_PARTICULE_AGH& particuleInfo, double &rnd1, double &rnd2, double &probability)
	{
		switch (materialInfo.reflectionLaw)
		{
		case REFLECTION_LAW_PHONG:
			return PhongSpecularPart(vectorDirection, faceNormal, particuleInfo, materialInfo, rnd1, rnd2, probability);
			break;
		default:
			return SpecularReflection(vectorDirection, faceNormal);
		};
	}


private:

	static vec3 PhongSpecularPart(vec3 &vectorDirection, vec3 &faceNormal, CONF_PARTICULE_AGH& particuleInfo, t_Material_BFreq &material, double &rnd1, double &rnd2, double &probability)
	{
		float n = powf(10, powf(-0.82662*material.diffusion, 3) + 1.5228);
		vec3 specularDir = SpecularReflection(vectorDirection, faceNormal);

		Matrix3 rotationMatrix;
		rotationMatrix.calculateRotationMatrix(faceNormal, specularDir);

		vec3 target = BaseWnReflection(vectorDirection, faceNormal, n, particuleInfo, rnd1, rnd2, probability);
		target = rotationMatrix*target;

		double test = target.dot(faceNormal);

		//test if reflected ray is pointing into ground
		if (target.dot(faceNormal) >= M_PI / 2)
		{
			//if it is get new random direction
			target = PhongSpecularPart(vectorDirection, faceNormal, particuleInfo, material, rnd1, rnd2, probability);
		}

		return target / target.length();
	}


	static vec3 BaseWnReflection(vec3 &vecteurVitesse, vec3 &faceNormal, decimal expo, CONF_PARTICULE& particuleInfo, double &rnd1, double &rnd2, double &probability)
	{
		decimal theta = rnd1 * M_2PI;
		decimal phi = acos(pow((float)1 - rnd2, (float)(1. / (expo + 1.))));//pow((float)acos(1-GetRandValue()),(float)(1./(expo+1.)));

		probability = ((expo + 2) / (2 * M_PI)) * pow(cos(phi),expo);

		return BaseUniformReflection(vecteurVitesse, faceNormal, theta, phi);
	}


	static vec3 SpecularReflection(vec3 &vecteurVitesse, vec3 &faceNormal)
	{
		vec3 retVal = (vecteurVitesse - (faceNormal*(vecteurVitesse.dot(faceNormal)) * 2));
		return retVal / retVal.length();
	}
};