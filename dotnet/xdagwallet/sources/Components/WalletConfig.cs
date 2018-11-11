using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Components
{
    public class WalletConfig
    {
        private static readonly string defaultConfigFileName = @"wallet-config.json";

        private static WalletConfig instance = null;

        public static WalletConfig Current
        {
            get
            {
                if (instance == null)
                {
                    instance = WalletConfig.ReadFromFile();
                }

                return instance;
            }
        }

        public WalletConfig()
        {
            this.Options = WalletOptions.Default();
        }

        public static WalletConfig ReadFromFile()
        {
            var location = System.Reflection.Assembly.GetExecutingAssembly().Location;
            var directoryPath = Path.GetDirectoryName(location);

            using (StreamReader sr = new StreamReader(Path.Combine(directoryPath, defaultConfigFileName)))
            {
                string jsonString = sr.ReadToEnd();
                WalletConfig config = JsonConvert.DeserializeObject<WalletConfig>(jsonString);
                return config;
            }
        }


        public void SaveToFile()
        {
            var location = System.Reflection.Assembly.GetExecutingAssembly().Location;
            var directoryPath = Path.GetDirectoryName(location);

            using (StreamWriter sw = new StreamWriter(Path.Combine(directoryPath, defaultConfigFileName)))
            {
                string content = JsonConvert.SerializeObject(this, Formatting.Indented);
                sw.Write(content);
            }
        }

        #region Properties

        [JsonProperty(PropertyName = "wallet_options")]
        public WalletOptions Options
        {
            get; set;
        }

        [JsonProperty(PropertyName = "version")]
        public string Version
        {
            get; set;
        }

        [JsonProperty(PropertyName = "culture_info")]
        public string CultureInfo
        {
            get; set;
        }
        
        [JsonIgnore]
        public string ReadOrDefaultCultureInfo
        {
            get
            {
                string cultureInfo = this.CultureInfo;
                if (string.IsNullOrEmpty(cultureInfo))
                {
                    cultureInfo = "en-US";
                }

                return cultureInfo;
            }
        }

        #endregion

    }
}
