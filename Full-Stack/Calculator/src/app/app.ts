import { Component, signal } from '@angular/core';
import { RouterOutlet } from '@angular/router';

@Component({
  selector: 'app-root',
  imports: [RouterOutlet],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class App {
  protected readonly title = signal('Calculator');

  displayValue : string = ''
  firstOperand : number | null = null
  operator : string | null = null
  firstNumDone: boolean = false;

  onClickNumber(val: string): void {
    this.displayValue += val
  }

  onSelectOperator(op: string): void {
    if(!this.firstNumDone && this.displayValue !== '') {
      this.firstOperand = Number(this.displayValue)
      this.displayValue = ''
      this.operator = op
      this.firstNumDone = true
    }
  }

  onCalculate(): void {
if (this.firstNumDone && this.operator !== null && this.displayValue !== '' && this.firstOperand !== null) {
      const secondOperand = Number(this.displayValue);
      let result = 0;

      switch (this.operator) {
        case '+': 
          result = this.firstOperand + secondOperand;
          break;
        case '-': 
          result = this.firstOperand - secondOperand;
          break;
        case '*': 
          result = this.firstOperand * secondOperand;
          break;
        case '/': 
          result = secondOperand !== 0 ? this.firstOperand / secondOperand : 0;
          break;
      }

      this.displayValue = String(result);

      this.firstOperand = null;
      this.operator = null;
      this.firstNumDone = false;
    }
  }

  onClear(): void {
    this.displayValue = ''
    this.firstOperand = null
    this.operator = null
    this.firstNumDone = false
  }
}
