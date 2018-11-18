using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Components
{
    public class XDagTransaction
    {
        /// <summary>
        /// This is the direction of the transaction defined in XDag
        /// </summary>
        public enum Directions
        {
            Input,
            Output,
            Fee,
        }

        /// <summary>
        /// 
        /// </summary>
        public enum TransactionStatus
        {
            Accepted,
            Pending,
        }

        public string BlockAddress
        {
            get; set;
        }

        public string PartnerAddress
        {
            get; set;
        }

        public Directions Direction
        {
            get; set;
        }

        public double Amount
        {
            get; set;
        }

        public DateTime TimeStamp
        {
            get; set;
        }

        public TransactionStatus Status
        {
            get; set;
        }

        public void SetDirection(string directionString)
        {
            switch(directionString.ToLower())
            {
                case "input":
                    this.Direction = Directions.Input;
                    break;
                case "output":
                    this.Direction = Directions.Output;
                    break;
                case "fee":
                    this.Direction = Directions.Fee;
                    break;
                default:
                    this.Direction = Directions.Output;
                    break;
            }
        }

        public void SetStatus(string statusString)
        {
            if (statusString.Equals("accepted", StringComparison.InvariantCultureIgnoreCase))
            {
                this.Status = TransactionStatus.Accepted;
            }

            if (statusString.Equals("pending", StringComparison.InvariantCultureIgnoreCase))
            {
                this.Status = TransactionStatus.Pending;
            }
        }
    }
}
