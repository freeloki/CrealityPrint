#include "us/usettings.h"
#include "us/defaultloader.h"
#include "us/settingdef.h"
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QSettings>
#include "internal/parameter/parameterpath.h"

namespace us
{
	USettings::USettings(QObject* parent)
		: QObject(parent)
		, m_def(nullptr)
	{
		m_def = &SettingDef::instance();
	}

	USettings::~USettings()
	{
	}

	USettings* USettings::copy()
	{
		USettings* settings = new USettings();
		settings->merge(this);
		return settings;
	}

	void USettings::insert(USetting* setting)
	{
		if (setting)
		{
			setting->setParent(this);
			QHash<QString, us::USetting*>::iterator insertIt = m_hashsettings.find(setting->key());
			if (insertIt != m_hashsettings.end())
			{
				insertIt.value() = setting;
			}
			else
			{
				m_hashsettings.insert(setting->key(), setting);
			}
		}
	}

	void USettings::add(const QString& key, const QString& value, bool cover)
	{
		QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			if (cover)
				it.value()->setValue(value);
		}
		else
		{
			USetting* s = SETTING2(key, value);
			m_hashsettings.insert(key, s);
		}
	}

	void USettings::merge(USettings* settings)
	{
		if (settings)
		{
			const QHash<QString, USetting*>& mergeSettings = settings->m_hashsettings;
			for (QHash<QString, us::USetting*>::const_iterator it = mergeSettings.begin(); it != mergeSettings.end(); ++it)
			{
				QHash<QString, us::USetting*>::iterator insertIt = m_hashsettings.find(it.key());
				if (insertIt != m_hashsettings.end())
				{
					insertIt.value()->setValue(it.value()->str());   // replace value
				}
				else
				{
					USetting* setting = it.value()->clone();
					setting->setParent(this);
					m_hashsettings.insert(it.key(), setting);     //insert
				}
			}
		}
	}

	void USettings::merge(const QHash<QString, QString>& kvs)
	{
		for (QHash<QString, QString>::const_iterator it = kvs.begin();
			it != kvs.end(); ++it)
		{
			QHash<QString, us::USetting*>::iterator insertIt = m_hashsettings.find(it.key());
			if (insertIt != m_hashsettings.end())
			{
				insertIt.value()->setValue(it.value());   // replace value
			}
			else
			{
				USetting* setting = SETTING2(it.key(), it.value());
				setting->setParent(this);
				m_hashsettings.insert(it.key(), setting);     //insert
			}
		}
	}

	void USettings::mergeNewItem(USettings* settings)
	{
		if (settings)
		{
			const QHash<QString, USetting*>& mergeSettings = settings->m_hashsettings;
			for (QHash<QString, us::USetting*>::const_iterator it = mergeSettings.begin(); it != mergeSettings.end(); ++it)
			{
				QHash<QString, us::USetting*>::iterator insertIt = m_hashsettings.find(it.key());
				if (insertIt != m_hashsettings.end())
				{
					//insertIt.value()->setValue(it.value()->value());   // replace value
				}
				else
				{
					USetting* setting = it.value()->clone();
					setting->setParent(this);
					m_hashsettings.insert(it.key(), setting);     //insert
				}
			}
		}
	}


    void USettings::clear()
    {
        m_hashsettings.clear();
    }
	
	void USettings::deleteValueByKey(const QString& key)
	{
		m_hashsettings.remove(key);
	}

    const QHash<QString, USetting*>& USettings::settings() const
	{
		return m_hashsettings;
	}

	void USettings::print()
	{
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
		{
			qDebug() << it.key() << " : " << it.value()->str();
		}
	}

	void USettings::loadDefault(const QString& file)
	{
		DefaultLoader loader;
		loader.loadDefault(file, this);
	}

	void USettings::appendDefault(const QStringList& keys)
	{
		for (const QString& key : keys)
		{
			if (!m_hashsettings.contains(key))
			{
				insert(us::SettingDef::instance().create(key));
			}
		}
	}

	void USettings::appendDefault()
	{
		QHash<QString, us::SettingItemDef*> hashItemDef = us::SettingDef::instance().getHashItemDef();
		for (QHash<QString, us::SettingItemDef*>::iterator it = hashItemDef.begin();
			it != hashItemDef.end(); ++it)
		{
			if (!m_hashsettings.contains(it.key()))
				m_hashsettings.insert(it.key(), us::SettingDef::instance().create(it.key()));
		}
	}

	void USettings::loadCompleted()
	{
		QHash<QString, us::SettingItemDef*> hashItemDef = us::SettingDef::instance().getHashItemDef();
		for (QHash<QString, us::SettingItemDef*>::iterator it = hashItemDef.begin();
			it != hashItemDef.end(); ++it)
		{
			m_hashsettings.insert(it.key(), us::SettingDef::instance().create(it.key()));
		}
	}

	QString USettings::value(const QString& key, const QString& defaultValue) const
	{
		QString valueStr = defaultValue;
		auto it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			valueStr = it.value()->str();
		}
		else
		{
			//qDebug() << "USettings value" << key << " failed ! use default value " << defaultValue;
		}
		
		return valueStr;
	}

	QVariant USettings::vvalue(const QString& key, const QVariant& defaultValue) const
	{
		QVariant value = defaultValue;
		auto it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
		{
			value = QVariant(it.value()->str());
		}
		else
		{
			qDebug() << QString("USettings vvalue key [%1] failed! use default value [%2].")
				.arg(key).arg(defaultValue.toString());
		}

		return value;
	}

	QList<USetting*> USettings::filter(const QString& category, const QString& filter, bool professional)
	{
		QList<USetting*> settings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
		{
			us::SettingItemDef* def = it.value()->def();
			if (def->category == category || category=="")
			{
				bool levelOK = false;
				if ((professional && def->level <= 4)
					|| (!professional && def->level <= 2))
					levelOK = true;
				if (levelOK)
				{
					bool filterOK = true;
					if (!filter.isEmpty())
					{
						QString lKey = def->name.toLower();
						QString lFilter = filter.toLower();
						if (!lKey.contains(lFilter))
							filterOK = false;
					}
					if (filterOK
#if 0
						&& settings.count() < 4
#endif
						)
						settings.append(it.value());
				}
			}
		}

		auto variantLessThanByOrder = [](us::USetting* v1, us::USetting* v2)
		{
			return v1->def()->order < v2->def()->order;
		};
		std::sort(settings.begin(), settings.end(), variantLessThanByOrder);
		return settings;
	}

	QList<USetting*> USettings::materialParameters(const QString& categoy)
	{
		QList<USetting*> settings;
		QStringList keys = creative_kernel::loadMaterialKeys(categoy);
		for (const QString& key : keys)
		{
			QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.find(key);
			if (it != m_hashsettings.end())
				settings.append(it.value());
		}
		return settings;
	}

	QList<USetting*> USettings::extruderParameters(const QString& categoy, const bool& professional)
	{
		QList<USetting*> settings;
		QStringList keys = creative_kernel::loadExtruderKeys(categoy);
		for (const QString& key : keys)
		{
			QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.find(key);
			if (it == m_hashsettings.end()) {
				continue;
			}

			us::SettingItemDef* def = it.value()->def();
			if ((professional && def->level <= 3)
				|| (!professional && def->level <= 2)) {
				settings.append(it.value());
			}
		}
		return settings;
	}

	QList<USetting*> USettings::machineParameters(const QString& categoy, const QString& subCategory)
	{
		QList<USetting*> settings;
		QStringList keys = creative_kernel::loadMachineKeys(categoy);
		for (const QString& key : keys)
		{
			if (!subCategory.isEmpty() && !key.contains(subCategory))
			{
				continue;
			}
			if (m_hashsettings.contains(key))
			{
				settings.append(m_hashsettings[key]);
			}
		}
		return settings;
	}

	USetting* USettings::findSetting(const QString& key)
	{
		QHash<QString, USetting*>::iterator it = m_hashsettings.find(key);
		if (it != m_hashsettings.end())
			return it.value();
		
		qDebug() << QString("USettings findSetting [%1] failed!.").arg(key);
		return nullptr;
	}
	bool  USettings::isEmpty()
	{
		return m_hashsettings.size()==0;
	}

	bool USettings::hasKey(const QString& key)
	{
		return m_hashsettings.constFind(key) != m_hashsettings.constEnd();
	}

	void USettings::update(const USettings* settings)
	{
		if (!settings) return;
		for (auto& ms : m_hashsettings)
		{
			QString val = settings->value(ms->key(), "");
			if (val != "")
			{
				ms->setValue(std::move(val));
			}
		}
	}

    void USettings::saveAsDefault(const QString& fileName)
    {
		{
			QFile file(fileName);
			file.remove();
		}

		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			qDebug() << QString("USettings::saveAsDefault [%1] failed.").arg(fileName);
			return;
		}

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		std::sort(orderSettings.begin(), orderSettings.end(), f);
		for(us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}

			QString str = QString("%1=%2\n").arg(key, v);
			file.write(str.toUtf8());
		}
		file.close();
    }

	void USettings::saveAsDefaultIni(const QString& fileName)
	{
		QSettings iniSetting(fileName, QSettings::IniFormat);

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		QStringList materialKeys = creative_kernel::loadMaterialKeys();
		QStringList extruderKeys = creative_kernel::loadExtruderKeys("");

		std::sort(orderSettings.begin(), orderSettings.end(), f);
		for (us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}

			if (materialKeys.contains(key))
			{
				iniSetting.setValue(QString("/Material/%1").arg(key), v);
			}
			else if (extruderKeys.contains(key))
			{
				iniSetting.setValue(QString("/Extruder/%1").arg(key), v);
			}
			else
			{
				iniSetting.setValue(QString("/Default/%1").arg(key), v);
			}
		}
	}

	void USettings::saveMachineAsDefault(const QString& fileName)
	{
		{
			QFile file(fileName);
			file.remove();
		}

		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			qDebug() << QString("USettings::saveAsDefault [%1] failed.").arg(fileName);
			return;
		}

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		std::sort(orderSettings.begin(), orderSettings.end(), f);
		file.write("[machine]\n");
		for (us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}

			QString str = QString("%1=%2\n").arg(key, v);
			file.write(str.toUtf8());
		}
		file.close();
	}

	void USettings::saveExtruderAsDefault(const QString& fileName, int index)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
		{
			qDebug() << QString("USettings::saveAsDefault [%1] failed.").arg(fileName);
			return;
		}

		QList<us::USetting*> orderSettings;
		for (QHash<QString, us::USetting*>::const_iterator it = m_hashsettings.begin();
			it != m_hashsettings.end(); ++it)
			orderSettings.append(it.value());

		auto f = [](us::USetting* s1, us::USetting* s2)->bool {
			return s1->def()->order < s2->def()->order;
		};

		std::sort(orderSettings.begin(), orderSettings.end(), f);
		file.write(QString("[Extruder_%1]\n").arg(index).toUtf8());
		for (us::USetting* setting : orderSettings)
		{
			QString v = setting->str();
			QString key = setting->key();

			if ("gcode_start" == key
				|| "inter_layer" == key
				|| "gcode_end" == key
				|| "machine_start_gcode" == key
				|| "machine_extruder_start_code" == key
				|| "machine_extruder_end_code" == key
				|| "machine_end_gcode" == key)
			{
				v = v.replace("\n", "\\n");
			}

			QString str = QString("%1=%2\n").arg(key, v);
			file.write(str.toUtf8());
		}
		file.close();
	}

	SettingDef* USettings::def()
	{
		return m_def;
	}

	QVariantList USettings::parameterGroupList()
	{
		return m_def->parameterGroupList();
	}

	QVariantList USettings::parameterList(const QString& type)
	{
		return m_def->parameterList(type);
	}

	QVariantList USettings::profileParameterList(int index)
	{
		return m_def->profileParameterList(index);
	}

	QObject* USettings::settingObject(const QString& key)
	{
		USetting* setting = findSetting(key);
		return setting;
	}
}
