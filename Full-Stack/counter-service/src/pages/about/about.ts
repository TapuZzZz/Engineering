import { Component, inject } from '@angular/core';
import { Counter } from '../../services/counter';

@Component({
  imports: [],
  selector: 'app-about',
  styleUrl: './about.css',
  templateUrl: './about.html',
})
export class About {
  counterService: Counter = inject(Counter);
}
