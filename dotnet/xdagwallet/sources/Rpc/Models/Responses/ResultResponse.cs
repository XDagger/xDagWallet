using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XDagNetWallet.Rpc.Models.Responses;

namespace XDagNetWallet.Rpc.Models
{
    public class ResultResponse<T> where T : ModelBase
    {
        public ResultResponse()
        {
        }

        [JsonProperty(PropertyName = "id")]
        public int Identifier
        {
            get; set;
        }

        [JsonProperty(PropertyName = "result")]
        public T Result
        {
            get; set;
        }

        [JsonProperty(PropertyName = "error")]
        public ErrorResponse Error
        {
            get; set;
        }
    }
}
