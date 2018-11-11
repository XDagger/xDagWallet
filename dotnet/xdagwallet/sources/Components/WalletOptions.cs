using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Components
{
    public class WalletOptions
    {
        [JsonProperty(PropertyName = "rpc_enabled")]
        public bool RpcEnabled
        {
            get; set;
        }

        [JsonProperty(PropertyName = "rpc_port")]
        public int? RpcPort
        {
            get; set;
        }

        [JsonProperty(PropertyName = "specified_pool_address")]
        public string SpecifiedPoolAddress
        {
            get; set;
        }

        [JsonProperty(PropertyName = "is_test_net")]
        public bool IsTestNet
        {
            get; set;
        }

        [JsonProperty(PropertyName = "disable_mining")]
        public bool DisableMining
        {
            get; set;
        }

        public WalletOptions()
        {

        }

        public static WalletOptions Default()
        {
            WalletOptions options = new WalletOptions();

            options.IsTestNet = false;
            options.DisableMining = true;
            options.RpcEnabled = false;

            return options;
        }

        public static WalletOptions TestNet()
        {
            WalletOptions options = Default();
            options.IsTestNet = true;

            return options;
        }
    }
}
