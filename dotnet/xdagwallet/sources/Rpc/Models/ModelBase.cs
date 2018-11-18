using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Rpc.Models
{
    public class ModelBase
    {
        public string ToJsonString()
        {
            return JsonConvert.SerializeObject(this, Formatting.None);
        }
    }
}
